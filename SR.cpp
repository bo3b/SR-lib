#include "SR.hpp"

#include <d3d9.h>
#include <d3d11_1.h>
#include <string>

#include "sr/management/srcontext.h"
#include "sr/weaver/dx9weaver.h"
#include "sr/weaver/dx11weaver.h"
#include "sr/weaver/glweaver.h"

using SimulatedReality::SRInterfaceDX11;
using SimulatedReality::SRInterfaceDX9;
using SimulatedReality::SRInterfaceOGL;


// Libraries used are set in Project Properties.  We specifically
// do not use the Debug versions here because at an end-user
// runtime, they will not be available.
// 
// Libraries:
//  x32:
//   opencv_world343.lib;
//   SimulatedRealityCore32.lib;
//   SimulatedRealityDirectX32.lib;
//  x64:
//   opencv_world343.lib;
//   SimulatedRealityCore.lib;
//   SimulatedRealityDirectX.lib;
// 
// Paths:
//  x32:
//   $(ProjectDir)SR-SDK-1.33.1\x32\third_party\OpenCV\lib\x86
//   $(ProjectDir)SR-SDK-1.33.1\x32\lib
//  x64
//   $(ProjectDir)SR-SDK-1.33.1\x64\third_party\OpenCV\lib\x64
//   $(ProjectDir)SR-SDK-1.33.1\x64\lib

// Comments from earlier version.
// 
// Only link against libs that are used.  The SDK is x32 and x64 now.
// These are for reference only here, and were moved to librarian input.
// They are now marked as DelayLoad, so that we don't have a hard DLL
// requirement for these, and only loaded when:
//  direct_mode = simulated_reality
// We will get benign duplicate symbol warnings, but need these:
//#pragma comment(lib, "SimulatedRealityCore.lib")
//#pragma comment(lib, "SimulatedRealityDirectX.lib")
//#pragma comment(lib, "opencv_world343.lib")
// Note- We never want to use #pragma comment for these, it's bad practice.

// 7-2-25
// Now moving the delay load of the DLLs here, and removed from the main
// geo-11 code in HackerDXGI during setup.  We want to be able to ship geo-11
// without any dangling dlls like the prior SpatialLabs3D.dll, but still
// allow people to use direct_mode = simulated_reality if they have the
// hardware and installed SR dlls.
// That means making this a static lib that is included with geo-11, but
// in here we do the LoadLibrary of the SR dlls needed.

// 2-23-26
// Completely revised to allow for DX9 support now too.  Should work for all
// variants, DX9, DX11, x32, x64.
//
// Decided to completely revamp the interface, taking a different tack that is
// not C++ like. The idea is to be more COM like, because that is ultimately
// our API, and doing stuff the C++ 'way' adds conflicts that are not very
// valuable.
// This approach is now to have a factory Create routine, and the normal COM
// style Release. This removes the invisible ctor and dtor side effects, and
// feels more natural to use in DirectX world.

#define SAFE_DELETE(p)  \
    {                   \
        if (p)          \
        {               \
            delete (p); \
            (p) = NULL; \
        }               \
    }

//-------------------------------------------------------------------------
// Sort of like member variables for SRInterfaceBase, but we don't want these
// to ever be visible in the SR.h header file for SDK use.

static SR::SRContext*            srContext_    = nullptr;
static SR::PredictingDX9Weaver*  srWeaverDX9_  = nullptr;
static SR::PredictingDX11Weaver* srWeaverDX11_ = nullptr;
static SR::PredictingGLWeaver*   srWeaverOGL_  = nullptr;

static unsigned int renderWidth_ {};
static unsigned int renderHeight_ {};

//-------------------------------------------------------------------------
// Any failures are considered fatal, so instead of doing any handling,
// we'll throw up MessageBox to let the user know. The user specified to
// use simulated_reality hardware, but it does not exist. There is no
// logical fallback, so let's just let them know.
static void FatalBox(std::wstring error, std::wstring title)
{
    ::MessageBoxW(nullptr, error.c_str(), title.c_str(), MB_OK);
    ::ExitProcess(-1);
}

//-------------------------------------------------------------------------
static void SetupSRContext()
{
    // Might get called more than once.
    if (srContext_ != nullptr)
        return;

    // Let's double check we are able find the SR DLLs we need for simulated reality,
    // so we can give a good error if not.
    HMODULE srCore_dll = nullptr;
#ifdef _WIN64
    srCore_dll = LoadLibrary(L"SimulatedRealityCore.dll");
#else
    srCore_dll = LoadLibrary(L"SimulatedRealityCore32.dll");
#endif
    if (srCore_dll == nullptr)
        FatalBox(L"Simulated Reality DLLs are not available on your system.\nUnable to use direct_mode=simulated_reality.", L"Missing SR runtime DLLs");

    srContext_ = new SR::SRContext();
    if (srContext_ == nullptr)
        FatalBox(L"Failed to create SRContext for simulated_reality.", L"Missing SR runtime DLLs");
}

//-------------------------------------------------------------------------
// DX9 interface to SR

HRESULT SimulatedReality::CreateSRInterfaceDX9(IDirect3DDevice9* device, unsigned int width, unsigned int height, HWND window, SRInterfaceDX9** ppReturnedSRInterfaceDX9)
{
    SetupSRContext();

    renderWidth_  = width;
    renderHeight_ = height;
    srWeaverDX9_  = new SR::PredictingDX9Weaver(*srContext_, device, renderWidth_, renderHeight_, window);

    // Generally never expect to be more than 1 frame late
    srWeaverDX9_->setLatencyInFrames(1);

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    srContext_->initialize();

    *ppReturnedSRInterfaceDX9 = new SRInterfaceDX9();

    return S_OK;
}

void SRInterfaceDX9::Release()
{
    SAFE_DELETE(srWeaverDX9_);

    if (srWeaverDX9_ == nullptr && srWeaverDX11_ == nullptr && srWeaverOGL_ == nullptr)
        SAFE_DELETE(srContext_);

    delete this;
}

// Returns the offscreen target that SR uses for weaving. We copy all of our
// stereo bits into it via StretchRect, at Present.
void SRInterfaceDX9::GetSRRenderSurface(IDirect3DSurface9** renderTarget)
{
    *renderTarget = srWeaverDX9_->getFrameBuffer();
}

void SRInterfaceDX9::PerformWeave()
{
    srWeaverDX9_->weave(renderWidth_, renderHeight_);
}

//-------------------------------------------------------------------------
// DX11 interface to SR

// Specifically targeting ID3D11Device1, because that is the last version supported on Windows 7.
// We want to be able to support running DX11 with Compatibility set to Win7 support, for the
// weirdo games that might need that.

HRESULT SimulatedReality::CreateSRInterfaceDX11(ID3D11Device1* device1, ID3D11DeviceContext1* context1, unsigned int width, unsigned int height, HWND window, SRInterfaceDX11** ppReturnedSRInterfaceDX11)
{
    SetupSRContext();

    renderWidth_  = width;
    renderHeight_ = height;
    srWeaverDX11_ = new SR::PredictingDX11Weaver(*srContext_, device1, context1, renderWidth_, renderHeight_, window);

    // Generally never expect to be more than 1 frame late
    srWeaverDX11_->setLatencyInFrames(1);

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    srContext_->initialize();

    *ppReturnedSRInterfaceDX11 = new SRInterfaceDX11();

    return S_OK;
}

void SRInterfaceDX11::Release()
{
    SAFE_DELETE(srWeaverDX11_);

    if (srWeaverDX9_ == nullptr && srWeaverDX11_ == nullptr && srWeaverOGL_ == nullptr)
        SAFE_DELETE(srContext_);

    delete this;
}

void SRInterfaceDX11::GetSRRenderSurface(ID3D11RenderTargetView** renderTarget)
{
    *renderTarget = srWeaverDX11_->getFrameBuffer();
}

void SRInterfaceDX11::PerformWeave()
{
    srWeaverDX11_->weave(renderWidth_, renderHeight_);
}

//-------------------------------------------------------------------------
// OpenGL interface to SR

// OpenGL has no device object to pass in; the weaver binds to the rendering
// context that is current on this thread when the weaver is constructed. The
// caller must therefore have a valid GL context current before calling this.

HRESULT SimulatedReality::CreateSRInterfaceOGL(unsigned int width, unsigned int height, HWND window, SRInterfaceOGL** ppReturnedSRInterfaceOGL)
{
    SetupSRContext();

    renderWidth_  = width;
    renderHeight_ = height;
    srWeaverOGL_  = new SR::PredictingGLWeaver(*srContext_, renderWidth_, renderHeight_, window);

    // Generally never expect to be more than 1 frame late
    srWeaverOGL_->setLatencyInFrames(1);

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    srContext_->initialize();

    *ppReturnedSRInterfaceOGL = new SRInterfaceOGL();

    return S_OK;
}

void SRInterfaceOGL::Release()
{
    SAFE_DELETE(srWeaverOGL_);

    if (srWeaverDX9_ == nullptr && srWeaverDX11_ == nullptr && srWeaverOGL_ == nullptr)
        SAFE_DELETE(srContext_);

    delete this;
}

// Returns the OpenGL framebuffer object that SR uses for weaving. Render the
// side-by-side stereo image into it, then call PerformWeave().
void SRInterfaceOGL::GetSRRenderSurface(GLuint* frameBuffer)
{
    *frameBuffer = srWeaverOGL_->getFrameBuffer();
}

void SRInterfaceOGL::PerformWeave()
{
    srWeaverOGL_->weave(renderWidth_, renderHeight_);
}
