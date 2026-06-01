#include "SR.hpp"

#include <d3d9.h>
#include <d3d11_1.h>
#include <GL/gl.h>
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
//   $(ProjectDir)SR-SDK-1.34.10\x32\third_party\OpenCV\lib\x86
//   $(ProjectDir)SR-SDK-1.34.10\x32\lib
//  x64
//   $(ProjectDir)SR-SDK-1.34.10\x64\third_party\OpenCV\lib\x64
//   $(ProjectDir)SR-SDK-1.34.10\x64\lib

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
// This approach is now to have a factory Create routine, paired with a
// Delete method. This removes the invisible ctor and dtor side effects, and
// feels more natural to use in DirectX world. Note Delete is not a COM-style
// refcounting Release; it unconditionally tears down the object.

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

static SR::SRContext* srContext_ = nullptr;

static SR::IDX9Weaver1*  srWeaverDX9_  = nullptr;
static SR::IDX11Weaver1* srWeaverDX11_ = nullptr;
static SR::IGLWeaver1*   srWeaverOGL_  = nullptr;

//-------------------------------------------------------------------------
static HRESULT SetupSRContext()
{
    // Might get called more than once.
    if (srContext_ != nullptr)
        return S_OK;

    // Let's double check we are able find the SR DLLs we need for simulated reality,
    // so we can return a good error if not.

    HMODULE sr_core_dll = nullptr;
#ifdef _WIN64
    sr_core_dll = LoadLibrary(L"SimulatedRealityCore.dll");
#else
    sr_core_dll = LoadLibrary(L"SimulatedRealityCore32.dll");
#endif
    if (sr_core_dll == nullptr)
        return E_NOINTERFACE;

    srContext_ = new SR::SRContext();
    if (srContext_ == nullptr)
        return E_NOINTERFACE;

    return S_OK;
}

//-------------------------------------------------------------------------
// DX9 interface to SR
//-------------------------------------------------------------------------
//   If we cannot successfully create a new IDX9Weaver1 object, return
//   E_NOINTERFACE as the hardware does not currently support SR output.

HRESULT SimulatedReality::CreateSRInterfaceDX9(IDirect3DDevice9* device, HWND window, SRInterfaceDX9** ppReturnedSRInterfaceDX9)
{
    HRESULT hr = SetupSRContext();
    if (FAILED(hr))
        return hr;

    WeaverErrorCode err = SR::CreateDX9Weaver(srContext_, device, window, &srWeaverDX9_);
    if (err != WeaverSuccess || srWeaverDX9_ == nullptr)
        return E_NOINTERFACE;

    // Generally never expect to be more than 1 frame late.
    // If frame rate drops below monitor refresh this can be off, but
    // generally 3D people try to keep the framerate up.
    // Can theoretically improve crosstalk.
    srWeaverDX9_->setLatencyInFrames(1);
    srWeaverDX9_->enableLateLatching(true);

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    srContext_->initialize();

    *ppReturnedSRInterfaceDX9 = new SRInterfaceDX9();

    return S_OK;
}

void SRInterfaceDX9::Delete()
{
    if (srWeaverDX9_ != nullptr)
    {
        srWeaverDX9_->destroy();
        srWeaverDX9_ = nullptr;
    }

    if (srWeaverDX9_ == nullptr && srWeaverDX11_ == nullptr && srWeaverOGL_ == nullptr)
        SAFE_DELETE(srContext_);

    delete this;
}

// Hand the weaver the side-by-side stereo texture. The weaver samples
// from it during Weave(). This is not done every frame.
void SRInterfaceDX9::SetInputTexture(IDirect3DTexture9* texture, bool isSRGB)
{
    D3DSURFACE_DESC desc = {};
    texture->GetLevelDesc(0, &desc);

    srWeaverDX9_->setInputViewTexture(texture, desc.Width, desc.Height, desc.Format, isSRGB);
}

void SRInterfaceDX9::Weave()
{
    srWeaverDX9_->weave();
}

//-------------------------------------------------------------------------
// DX11 interface to SR
//-------------------------------------------------------------------------
//  The 1.34.10 weaver takes a D3D11 immediate device context (it no longer
//  needs the device, and the older Device1/Context1 split is gone).

HRESULT SimulatedReality::CreateSRInterfaceDX11(ID3D11DeviceContext* context, HWND window, SRInterfaceDX11** ppReturnedSRInterfaceDX11)
{
    HRESULT hr = SetupSRContext();
    if (FAILED(hr))
        return hr;

    WeaverErrorCode err = SR::CreateDX11Weaver(srContext_, context, window, &srWeaverDX11_);
    if (err != WeaverSuccess || srWeaverDX11_ == nullptr)
        return E_NOINTERFACE;

    srWeaverDX11_->setLatencyInFrames(1);     // Generally never expect to be more than 1 frame late
    srWeaverDX11_->enableLateLatching(true);  // Can theoretically improve crosstalk

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    srContext_->initialize();

    *ppReturnedSRInterfaceDX11 = new SRInterfaceDX11();

    return S_OK;
}

void SRInterfaceDX11::Delete()
{
    if (srWeaverDX11_ != nullptr)
    {
        srWeaverDX11_->destroy();
        srWeaverDX11_ = nullptr;
    }

    if (srWeaverDX9_ == nullptr && srWeaverDX11_ == nullptr && srWeaverOGL_ == nullptr)
        SAFE_DELETE(srContext_);

    delete this;
}

// Hand the weaver the side-by-side stereo texture. We pull the size from the
// underlying resource and the format from the view (so the SRV's sRGB-ness is
// honored), matching the DX9 path where the caller only passes the texture.
void SRInterfaceDX11::SetInputTexture(ID3D11ShaderResourceView* texture)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
    texture->GetDesc(&view_desc);

    ID3D11Resource*  resource = nullptr;
    ID3D11Texture2D* tex_2D   = nullptr;
    texture->GetResource(&resource);
    {
        resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex_2D));
        D3D11_TEXTURE2D_DESC tex_desc = {};
        tex_2D->GetDesc(&tex_desc);
        srWeaverDX11_->setInputViewTexture(texture, tex_desc.Width, tex_desc.Height, view_desc.Format);
    }
    tex_2D->Release();
    resource->Release();
}

void SRInterfaceDX11::Weave()
{
    srWeaverDX11_->weave();
}

//-------------------------------------------------------------------------
// OpenGL interface to SR
//-------------------------------------------------------------------------
//  OpenGL has no device object to pass in; the weaver binds to the rendering
//  context that is current on this thread when the weaver is constructed. The
//  caller must therefore have a valid GL context current before calling this.

HRESULT SimulatedReality::CreateSRInterfaceOGL(HWND window, SRInterfaceOGL** ppReturnedSRInterfaceOGL)
{
    HRESULT hr = SetupSRContext();
    if (FAILED(hr))
        return hr;

    WeaverErrorCode err = SR::CreateGLWeaver(*srContext_, window, &srWeaverOGL_);
    if (err != WeaverSuccess || srWeaverOGL_ == nullptr)
        return E_NOINTERFACE;

    srWeaverOGL_->setLatencyInFrames(1);     // Generally never expect to be more than 1 frame late
    srWeaverOGL_->enableLateLatching(true);  // Can theoretically improve crosstalk

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    srContext_->initialize();

    *ppReturnedSRInterfaceOGL = new SRInterfaceOGL();

    return S_OK;
}

void SRInterfaceOGL::Delete()
{
    if (srWeaverOGL_ != nullptr)
    {
        srWeaverOGL_->destroy();
        srWeaverOGL_ = nullptr;
    }

    if (srWeaverDX9_ == nullptr && srWeaverDX11_ == nullptr && srWeaverOGL_ == nullptr)
        SAFE_DELETE(srContext_);

    delete this;
}

// Hand the weaver the side-by-side stereo texture. We query the size and
// internal format straight from the GL texture object (which encodes sRGB),
// matching the DX9 path where the caller only passes the texture. This needs a
// current GL context, which the caller already has for the weaver. We save and
// restore the 2D binding so we don't disturb the caller's GL state.
void SRInterfaceOGL::SetInputTexture(GLuint texture)
{
    GLint prev_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    GLint width = 0, height = 0, internal_format = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prev_texture));

    srWeaverOGL_->setInputViewTexture(texture, width, height, static_cast<GLenum>(internal_format));
}

void SRInterfaceOGL::Weave()
{
    srWeaverOGL_->weave();
}
