#include "SR.hpp"

#include <d3d9.h>
#include <d3d11_1.h>
#include <d3d12.h>
#include <GL/gl.h>
#include <mutex>
#include <string>

#include "sr/management/srcontext.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/display/switchablehint.h"
#include "sr/sense/eyetracker/eyepairlistener.h"
#include "sr/sense/eyetracker/eyepairstream.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/sense/headtracker/head.h"
#include "sr/sense/headtracker/headlistener.h"
#include "sr/sense/headtracker/headposelistener.h"
#include "sr/sense/headtracker/headposestream.h"
#include "sr/sense/headtracker/headposetracker.h"
#include "sr/sense/headtracker/headtracker.h"
#include "sr/weaver/dx9weaver.h"
#include "sr/weaver/dx11weaver.h"
#include "sr/weaver/dx12weaver.h"
#include "sr/weaver/glweaver.h"

using SimulatedReality::SRInterfaceDX11;
using SimulatedReality::SRInterfaceDX12;
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
//   SimulatedRealityDisplays32.lib;      (SwitchableLensHint)
//   SimulatedRealityFaceTrackers32.lib;  (Head/HeadPose/Eye trackers)
//  x64:
//   opencv_world343.lib;
//   SimulatedRealityCore.lib;
//   SimulatedRealityDirectX.lib;
//   SimulatedRealityDisplays.lib;        (SwitchableLensHint)
//   SimulatedRealityFaceTrackers.lib;    (Head/HeadPose/Eye trackers)
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


//-------------------------------------------------------------------------
// Sort of like member variables for SRInterfaceBase, but we don't want these
// to ever be visible in the SR.h header file for SDK use.

static SR::SRContext* srContext_ = nullptr;

static SR::IDX9Weaver1*  srWeaverDX9_  = nullptr;
static SR::IDX11Weaver1* srWeaverDX11_ = nullptr;
static SR::IDX12Weaver1* srWeaverDX12_ = nullptr;
static SR::IGLWeaver1*   srWeaverOGL_  = nullptr;

// Owned by srContext_ once created — we only null the pointer, never delete it.
static SR::SwitchableLensHint* srLensHint_        = nullptr;
static bool                    srLensHintTried_   = false;
// The preference we last sent, so a per-frame caller doesn't re-send it. Resets
// wherever srLensHint_ does: the hint dies with its context, and the service
// drops our preference along with the session.
static bool                    srLensHintEnabled_ = false;

//-------------------------------------------------------------------------
// Tracking
//-------------------------------------------------------------------------
//  The SDK pushes tracking at us on its own thread, one accept() call per
//  captured frame. Consumers overwhelmingly want to *pull* the latest value
//  when they render, so each listener below just caches the newest frame under
//  a lock and the SRGet* functions read it back. That is the whole reason this
//  exists: it is the boilerplate every SR consumer writes identically.
//
//  Frames are copied wholesale rather than field-by-field — SR_head and friends
//  are PODs, so the copy is cheap and can't miss a field the SDK adds later.

namespace {

template <typename FrameType>
class FrameCache
{
    std::mutex mutex_;
    FrameType  frame_{};
    bool       valid_ = false;

public:
    void store(const FrameType& f)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        frame_ = f;
        valid_ = true;
    }

    bool load(FrameType& out)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!valid_)
            return false;
        out = frame_;
        return true;
    }
};

class HeadCache : public SR::HeadListener
{
public:
    SR::InputStream<SR::HeadStream> stream;
    FrameCache<SR_head>             cache;

    void accept(const SR_head& frame) override { cache.store(frame); }
};

class HeadPoseCache : public SR::HeadPoseListener
{
public:
    SR::InputStream<SR::HeadPoseStream> stream;
    FrameCache<SR_headPose>             cache;

    void accept(const SR_headPose& frame) override { cache.store(frame); }
};

class EyePairCache : public SR::EyePairListener
{
public:
    SR::InputStream<SR::EyePairStream> stream;
    FrameCache<SR_eyePair>             cache;

    void accept(const SR_eyePair& frame) override { cache.store(frame); }
};

void CopyVec(const SR_point3d& in, SimulatedReality::SRVec3& out)
{
    out.x = in.x;
    out.y = in.y;
    out.z = in.z;
}

}  // namespace

static unsigned int srTrackingRequested_ = SimulatedReality::SR_TRACK_NONE;
static unsigned int srTrackingActive_    = SimulatedReality::SR_TRACK_NONE;

static SR::HeadTracker*     srHeadTracker_     = nullptr;
static SR::HeadPoseTracker* srHeadPoseTracker_ = nullptr;
static SR::EyeTracker*      srEyeTracker_      = nullptr;

static HeadCache*     srHeadCache_     = nullptr;
static HeadPoseCache* srHeadPoseCache_ = nullptr;
static EyePairCache*  srEyePairCache_  = nullptr;

//-------------------------------------------------------------------------
// Called from every CreateSRInterface*, after the weaver exists but BEFORE
// srContext_->initialize(). Senses have to be registered with the context
// while it is still being built; the SDK's own samples do the same.
//
// A tracker that won't start (no camera, unsupported hardware) is dropped
// rather than failing the whole interface creation — losing head tracking is
// survivable, losing the weave is not. SRActiveTracking() reports what really
// came up.
static void StartRequestedTracking()
{
    using namespace SimulatedReality;

    if (srContext_ == nullptr || srTrackingRequested_ == SR_TRACK_NONE)
        return;

    if (srTrackingRequested_ & SR_TRACK_HEAD)
    {
        try
        {
            srHeadTracker_ = SR::HeadTracker::create(*srContext_);
            srHeadCache_   = new HeadCache();
            srHeadCache_->stream.set(srHeadTracker_->openHeadStream(srHeadCache_));
            srTrackingActive_ |= SR_TRACK_HEAD;
        }
        catch (...)
        {
            delete srHeadCache_;
            srHeadCache_   = nullptr;
            srHeadTracker_ = nullptr;
        }
    }

    if (srTrackingRequested_ & SR_TRACK_HEAD_POSE)
    {
        try
        {
            srHeadPoseTracker_ = SR::HeadPoseTracker::create(*srContext_);
            srHeadPoseCache_   = new HeadPoseCache();
            srHeadPoseCache_->stream.set(srHeadPoseTracker_->openHeadPoseStream(srHeadPoseCache_));
            srTrackingActive_ |= SR_TRACK_HEAD_POSE;
        }
        catch (...)
        {
            delete srHeadPoseCache_;
            srHeadPoseCache_   = nullptr;
            srHeadPoseTracker_ = nullptr;
        }
    }

    if (srTrackingRequested_ & (SR_TRACK_EYES | SR_TRACK_EYES_RAW))
    {
        // RAW wins when both are set: a caller that explicitly asked for
        // unfiltered data is applying its own filter, and handing it the
        // smoothed feed instead would silently fight that.
        const bool raw = (srTrackingRequested_ & SR_TRACK_EYES_RAW) != 0;
        try
        {
            srEyeTracker_ = raw ? SR::EyeTracker::createRaw(*srContext_)
                                : SR::EyeTracker::create(*srContext_);
            srEyePairCache_ = new EyePairCache();
            srEyePairCache_->stream.set(srEyeTracker_->openEyePairStream(srEyePairCache_));
            srTrackingActive_ |= (raw ? SR_TRACK_EYES_RAW : SR_TRACK_EYES);
        }
        catch (...)
        {
            delete srEyePairCache_;
            srEyePairCache_ = nullptr;
            srEyeTracker_   = nullptr;
        }
    }
}

//-------------------------------------------------------------------------
// The trackers are Senses owned by the SRContext, so they go when it goes — we
// only drop our pointers. The listeners are ours: deleting one runs its
// InputStream destructor, which stops the stream, so this must happen before
// the context is destroyed.
static void StopTracking()
{
    delete srHeadCache_;
    srHeadCache_ = nullptr;
    delete srHeadPoseCache_;
    srHeadPoseCache_ = nullptr;
    delete srEyePairCache_;
    srEyePairCache_ = nullptr;

    srHeadTracker_     = nullptr;
    srHeadPoseTracker_ = nullptr;
    srEyeTracker_      = nullptr;

    srTrackingActive_ = SimulatedReality::SR_TRACK_NONE;

    // Re-arm: the next CreateSRInterface* starts from a clean request, so a
    // caller can choose different senses for a later session.
    srTrackingRequested_ = SimulatedReality::SR_TRACK_NONE;
}

//-------------------------------------------------------------------------
// The context outlives individual weavers, so it is only torn down once the
// last one has gone. Every Delete() routes through here so the four copies of
// this condition can't drift apart as backends are added.
static void ReleaseSRContextIfIdle()
{
    if (srWeaverDX9_ != nullptr || srWeaverDX11_ != nullptr ||
        srWeaverDX12_ != nullptr || srWeaverOGL_ != nullptr)
        return;

    // The hint is a Sense owned by the context; deleting the context takes it
    // with it. Just drop our pointer and re-arm the lazy create.
    srLensHint_      = nullptr;
    srLensHintTried_ = false;
    srLensHintEnabled_ = false;

    StopTracking();

    // Paired with SRContext::create() — see SetupSRContext.
    if (srContext_ != nullptr)
    {
        SR::SRContext::deleteSRContext(srContext_);
        srContext_ = nullptr;
    }
}

//-------------------------------------------------------------------------
// Bring the senses online. Called from every CreateSRInterface*, and the
// ordering around it is load-bearing: initialize() must run AFTER weaver
// creation or eye tracking silently never starts, while every call still
// reports success and the panel just shows an image that ignores head motion.
//
// initialize() is not documented as throwing, and the SDK's own DX11 / DX12 /
// OpenGL samples call it bare. But it is what starts every registered sense,
// and DeviceNotAvailableException exists for precisely that failure -- a camera
// unplugged, or claimed by another SR application, in the window between
// SRContext::create() and here. Whether or not it can throw today, letting it
// would send a C++ exception straight through an extern "C" HRESULT function
// whose entire contract is to report failure as a return code, into a caller
// that may not have exception handling enabled at all. Contain it here, where
// the context lifecycle already lives.
//
// The weaver teardown on failure belongs to the caller: only it knows which
// backend pointer to clear, and ReleaseSRContextIfIdle() keys off exactly those
// pointers.
static HRESULT InitializeSRContext()
{
    try
    {
        srContext_->initialize();
    }
    catch (...)
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
// Names differ by bitness; the 32-bit runtime suffixes every SR DLL with "32".
#ifdef _WIN64
#define SR_DLL_CORE    L"SimulatedRealityCore.dll"
#define SR_DLL_DIRECTX L"SimulatedRealityDirectX.dll"
#define SR_DLL_OPENGL  L"SimulatedRealityOpenGL.dll"
#else
#define SR_DLL_CORE    L"SimulatedRealityCore32.dll"
#define SR_DLL_DIRECTX L"SimulatedRealityDirectX32.dll"
#define SR_DLL_OPENGL  L"SimulatedRealityOpenGL32.dll"
#endif

// Probe a delay-loaded SR DLL. Every SR DLL is delay-loaded by the consumer, so
// the first call into one that isn't on disk raises an SEH exception from the
// delay-load helper — which a C++ catch can't intercept without compiling the
// whole TU /EHa. LoadLibraryW just returns null instead, so this is how we find
// out safely before touching any SDK entry point.
//
// Deliberately no FreeLibrary: another DLL already in the process may depend on
// what we just loaded, and we're about to use it ourselves anyway.
static bool srProbeDll(const wchar_t* name)
{
    return LoadLibraryW(name) != nullptr;
}

//-------------------------------------------------------------------------
// backendDll is the weaver DLL for the interface being created — SR_DLL_DIRECTX
// for DX9/11/12, SR_DLL_OPENGL for GL. It has to be probed per backend rather
// than once globally: a machine can have the core runtime present while the
// backend DLL is missing, and the core probe alone would wave that through to
// an SEH inside CreateXxxWeaver.
//
// Probing the backend DLL is also the strongest single check available. On this
// runtime SimulatedRealityDirectX.dll statically imports DimencoWeaving,
// SimulatedRealityCore, SimulatedRealityDisplays, SimulatedRealityFaceTrackers
// and opencv_world343, so a successful load resolves the whole chain. We still
// probe the core DLL explicitly rather than lean on that: the static dependency
// set is an implementation detail of a particular SDK build, and this costs one
// refcount bump.
static HRESULT SetupSRContext(const wchar_t* backendDll)
{
    // Checked on every create, including ones that reuse an existing context,
    // since each backend brings its own weaver DLL.
    if (!srProbeDll(backendDll))
        return E_NOINTERFACE;

    // Might get called more than once.
    if (srContext_ != nullptr)
        return S_OK;

    // Let's double check we are able find the SR DLLs we need for simulated reality,
    // so we can return a good error if not.
    //
    // LoadLibraryW explicitly rather than the LoadLibrary macro: the literals
    // here are already wide, so the macro only resolves correctly for consumers
    // that build with UNICODE defined. Naming the W entry point directly makes
    // this compile the same either way.
    if (!srProbeDll(SR_DLL_CORE))
        return E_NOINTERFACE;

    // SRContext::create() rather than `new SR::SRContext()`: the context's
    // implementation lives in the SR DLL, and create()/deleteSRContext() are the
    // matching pair for allocating it there. Constructing it with our own `new`
    // and destroying it with our own `delete` puts the object on the wrong heap.
    //
    // It also documents throwing ServerNotAvailableException when the SR Service
    // isn't running, which is an ordinary thing to happen on an end-user machine
    // — the DLLs can be present with the service stopped. Catch it here rather
    // than letting a C++ exception escape through an extern "C" HRESULT.
    try
    {
        srContext_ = SR::SRContext::create();
    }
    catch (...)
    {
        srContext_ = nullptr;
    }

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
    HRESULT hr = SetupSRContext(SR_DLL_DIRECTX);
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

    // Senses must be registered while the context is still being built, so this
    // sits between weaver creation and initialize(). No-op unless the caller
    // asked for tracking via SRRequestTracking().
    StartRequestedTracking();

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    hr = InitializeSRContext();
    if (FAILED(hr))
    {
        srWeaverDX9_->destroy();
        srWeaverDX9_ = nullptr;
        ReleaseSRContextIfIdle();
        return hr;
    }

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

    ReleaseSRContextIfIdle();

    delete this;
}

// Hand the weaver the side-by-side stereo texture. The weaver samples
// from it during Weave(). This is not done every frame.
void SRInterfaceDX9::SetInputTexture(IDirect3DTexture9* texture, bool isSRGB, bool outputSRGB)
{
    D3DSURFACE_DESC desc = {};
    texture->GetLevelDesc(0, &desc);

    srWeaverDX9_->setInputViewTexture(texture, desc.Width, desc.Height, desc.Format, isSRGB);
    srWeaverDX9_->setOutputSRGBWrite(outputSRGB);
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
    HRESULT hr = SetupSRContext(SR_DLL_DIRECTX);
    if (FAILED(hr))
        return hr;

    WeaverErrorCode err = SR::CreateDX11Weaver(srContext_, context, window, &srWeaverDX11_);
    if (err != WeaverSuccess || srWeaverDX11_ == nullptr)
        return E_NOINTERFACE;

    srWeaverDX11_->setLatencyInFrames(1);     // Generally never expect to be more than 1 frame late
    srWeaverDX11_->enableLateLatching(true);  // Can theoretically improve crosstalk

    // Senses must be registered while the context is still being built, so this
    // sits between weaver creation and initialize(). No-op unless the caller
    // asked for tracking via SRRequestTracking().
    StartRequestedTracking();

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    hr = InitializeSRContext();
    if (FAILED(hr))
    {
        srWeaverDX11_->destroy();
        srWeaverDX11_ = nullptr;
        ReleaseSRContextIfIdle();
        return hr;
    }

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

    ReleaseSRContextIfIdle();

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
// DX12 interface to SR
//-------------------------------------------------------------------------
//  Unlike DX9/DX11, the DX12 weaver has no device context to record into. It
//  records into a caller-supplied command list, so the command list and the
//  viewport/scissor are per-frame state rather than create-time state.

HRESULT SimulatedReality::CreateSRInterfaceDX12(ID3D12Device* device, HWND window, SRInterfaceDX12** ppReturnedSRInterfaceDX12)
{
    HRESULT hr = SetupSRContext(SR_DLL_DIRECTX);
    if (FAILED(hr))
        return hr;

    WeaverErrorCode err = SR::CreateDX12Weaver(srContext_, device, window, &srWeaverDX12_);
    if (err != WeaverSuccess || srWeaverDX12_ == nullptr)
        return E_NOINTERFACE;

    srWeaverDX12_->setLatencyInFrames(1);     // Generally never expect to be more than 1 frame late
    srWeaverDX12_->enableLateLatching(true);  // Can theoretically improve crosstalk

    // Senses must be registered while the context is still being built, so this
    // sits between weaver creation and initialize(). No-op unless the caller
    // asked for tracking via SRRequestTracking().
    StartRequestedTracking();

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    hr = InitializeSRContext();
    if (FAILED(hr))
    {
        srWeaverDX12_->destroy();
        srWeaverDX12_ = nullptr;
        ReleaseSRContextIfIdle();
        return hr;
    }

    *ppReturnedSRInterfaceDX12 = new SRInterfaceDX12();

    return S_OK;
}

void SRInterfaceDX12::Delete()
{
    if (srWeaverDX12_ != nullptr)
    {
        // IDX12Weaver1 is an IDestroyable — destroy(), never `delete`. Deleting
        // it is the older deprecated DX12Weaver convention and asserts in debug.
        srWeaverDX12_->destroy();
        srWeaverDX12_ = nullptr;
    }

    ReleaseSRContextIfIdle();

    delete this;
}

// Hand the weaver the side-by-side stereo texture. Size and format come off the
// resource desc rather than from the caller, matching the other backends — and
// removing any chance of describing the texture as a size it isn't, which the
// weaver would happily sample past the edge of.
void SRInterfaceDX12::SetInputTexture(ID3D12Resource* texture)
{
    D3D12_RESOURCE_DESC desc = texture->GetDesc();

    srWeaverDX12_->setInputViewTexture(texture,
                                       static_cast<int>(desc.Width),
                                       static_cast<int>(desc.Height),
                                       desc.Format);
}

void SRInterfaceDX12::SetOutputFormat(DXGI_FORMAT format)
{
    srWeaverDX12_->setOutputFormat(format);
}

void SRInterfaceDX12::SetCommandList(ID3D12GraphicsCommandList* commandList)
{
    srWeaverDX12_->setCommandList(commandList);
}

void SRInterfaceDX12::SetViewport(const D3D12_VIEWPORT& viewport)
{
    srWeaverDX12_->setViewport(viewport);
}

void SRInterfaceDX12::SetScissorRect(const D3D12_RECT& scissorRect)
{
    srWeaverDX12_->setScissorRect(scissorRect);
}

void SRInterfaceDX12::Weave()
{
    srWeaverDX12_->weave();
}

// Convenience overload for the usual per-frame case. Note the weaver's own
// setViewport() does not drive the D3D12 rasterizer — the caller must still
// have called RSSetViewports/RSSetScissorRects on the command list with the
// destination dimensions, or the weave rasterizes at whatever size the previous
// pass left behind. See the warning in SR.hpp.
void SRInterfaceDX12::Weave(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect)
{
    srWeaverDX12_->setCommandList(commandList);
    srWeaverDX12_->setViewport(viewport);
    srWeaverDX12_->setScissorRect(scissorRect);
    srWeaverDX12_->weave();
}

//-------------------------------------------------------------------------
// OpenGL interface to SR
//-------------------------------------------------------------------------
//  OpenGL has no device object to pass in; the weaver binds to the rendering
//  context that is current on this thread when the weaver is constructed. The
//  caller must therefore have a valid GL context current before calling this.

HRESULT SimulatedReality::CreateSRInterfaceOGL(HWND window, SRInterfaceOGL** ppReturnedSRInterfaceOGL)
{
    HRESULT hr = SetupSRContext(SR_DLL_OPENGL);
    if (FAILED(hr))
        return hr;

    WeaverErrorCode err = SR::CreateGLWeaver(*srContext_, window, &srWeaverOGL_);
    if (err != WeaverSuccess || srWeaverOGL_ == nullptr)
        return E_NOINTERFACE;

    srWeaverOGL_->setLatencyInFrames(1);     // Generally never expect to be more than 1 frame late
    srWeaverOGL_->enableLateLatching(true);  // Can theoretically improve crosstalk

    // Senses must be registered while the context is still being built, so this
    // sits between weaver creation and initialize(). No-op unless the caller
    // asked for tracking via SRRequestTracking().
    StartRequestedTracking();

    // Must be done after Weaver creation, otherwise eye tracking is broken.
    hr = InitializeSRContext();
    if (FAILED(hr))
    {
        srWeaverOGL_->destroy();
        srWeaverOGL_ = nullptr;
        ReleaseSRContextIfIdle();
        return hr;
    }

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

    ReleaseSRContextIfIdle();

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

//-------------------------------------------------------------------------
// Switchable lens hint
//-------------------------------------------------------------------------
//  Created lazily rather than inside SetupSRContext, for two reasons. First,
//  SwitchableLensHint::create must run AFTER srContext_->initialize(), which
//  each CreateSRInterface* does at the very end — by the time a caller can
//  reach these functions, initialize() has run. Second, consumers that never
//  touch the lens API then never construct the sense at all, so nothing about
//  the existing paths changes.
//
//  create() throws when the connected display has no switchable lens, which is
//  the common case on fixed-lens panels — so it is a normal outcome to report,
//  not an error to propagate. We latch the attempt either way so a display
//  without a lens doesn't re-throw on every focus change.

static SR::SwitchableLensHint* GetLensHint()
{
    if (srLensHintTried_)
        return srLensHint_;

    if (srContext_ == nullptr)
    {
        // No context yet — leave the latch clear so a call made before the
        // first CreateSRInterface* can't wedge the hint permanently.
        return nullptr;
    }

    srLensHintTried_ = true;

    try
    {
        srLensHint_ = SR::SwitchableLensHint::create(*srContext_);
    }
    catch (...)
    {
        srLensHint_ = nullptr;
    }

    return srLensHint_;
}

//  Consumers naturally drive the lens from per-frame state ("am I weaving right
//  now?"), so these have to be cheap and safe to call every frame. Two things
//  make them so, and both belong here rather than in each integration:
//
//   - Redundant calls never reach the SDK. srLensHintEnabled_ is OUR preference,
//     which is exactly what the service arbitrates on, so re-sending an
//     unchanged one buys nothing and puts an IPC round trip in the frame loop.
//     The caller learns which happened from S_OK vs S_FALSE, so it can log the
//     transition without keeping a shadow copy of the state.
//   - A display with no switchable lens costs one failed create, not one per
//     frame — GetLensHint latches that (srLensHintTried_) and returns nullptr
//     from then on.
static HRESULT SetLensHint(bool enable)
{
    if (srLensHintEnabled_ == enable)
        return S_FALSE;

    SR::SwitchableLensHint* hint = GetLensHint();
    if (hint == nullptr)
        return E_NOINTERFACE;

    // The SDK documents std::system_error out of the neighbouring lens calls
    // when their mutex lock fails, and this is an extern "C" HRESULT boundary —
    // the same reason SetupSRContext catches around SRContext::create.
    try
    {
        if (enable)
            hint->enable();
        else
            hint->disable();
    }
    catch (...)
    {
        return E_FAIL;
    }

    srLensHintEnabled_ = enable;
    return S_OK;
}

HRESULT SimulatedReality::SREnableLensHint()
{
    return SetLensHint(true);
}

HRESULT SimulatedReality::SRDisableLensHint()
{
    return SetLensHint(false);
}

HRESULT SimulatedReality::SRIsLensHintEnabled(bool* enabled)
{
    if (enabled == nullptr)
        return E_POINTER;

    *enabled = false;

    SR::SwitchableLensHint* hint = GetLensHint();
    if (hint == nullptr)
        return E_NOINTERFACE;

    *enabled = hint->isEnabled();
    return S_OK;
}

//-------------------------------------------------------------------------
// Shared weaver tuning
//-------------------------------------------------------------------------
//  All four weaver interfaces derive from IWeaverBase1, so these forward to one
//  implementation apiece rather than four copies that could drift. Each is
//  null-tolerant: calling a setter on an interface whose weaver has already
//  been destroyed is a no-op, not a crash.

namespace {

void WeaverSetSRGB(SR::IWeaverBase1* weaver, bool read, bool write)
{
    if (weaver != nullptr)
        weaver->setShaderSRGBConversion(read, write);
}

void WeaverSetLatencyInFrames(SR::IWeaverBase1* weaver, unsigned long long frames)
{
    if (weaver != nullptr)
        weaver->setLatencyInFrames(static_cast<uint64_t>(frames));
}

void WeaverSetLatency(SR::IWeaverBase1* weaver, unsigned long long microseconds)
{
    if (weaver != nullptr)
        weaver->setLatency(static_cast<uint64_t>(microseconds));
}

void WeaverEnableLateLatching(SR::IWeaverBase1* weaver, bool enable)
{
    if (weaver != nullptr)
        weaver->enableLateLatching(enable);
}

bool WeaverGetPredictedEyes(SR::IWeaverBase1* weaver, float left[3], float right[3])
{
    if (weaver == nullptr || left == nullptr || right == nullptr)
        return false;

    weaver->getPredictedEyePositions(left, right);
    return true;
}

}  // namespace

#define SR_DEFINE_WEAVER_KNOBS(InterfaceName, weaverGlobal)                                        \
    void InterfaceName::SetShaderSRGBConversion(bool read, bool write)                             \
    {                                                                                              \
        WeaverSetSRGB(weaverGlobal, read, write);                                                  \
    }                                                                                              \
    void InterfaceName::SetLatencyInFrames(unsigned long long frames)                              \
    {                                                                                              \
        WeaverSetLatencyInFrames(weaverGlobal, frames);                                            \
    }                                                                                              \
    void InterfaceName::SetLatency(unsigned long long microseconds)                                \
    {                                                                                              \
        WeaverSetLatency(weaverGlobal, microseconds);                                              \
    }                                                                                              \
    void InterfaceName::EnableLateLatching(bool enable)                                            \
    {                                                                                              \
        WeaverEnableLateLatching(weaverGlobal, enable);                                            \
    }                                                                                              \
    bool InterfaceName::GetPredictedEyePositions(float left[3], float right[3])                    \
    {                                                                                              \
        return WeaverGetPredictedEyes(weaverGlobal, left, right);                                  \
    }

SR_DEFINE_WEAVER_KNOBS(SRInterfaceDX9,  srWeaverDX9_)
SR_DEFINE_WEAVER_KNOBS(SRInterfaceDX11, srWeaverDX11_)
SR_DEFINE_WEAVER_KNOBS(SRInterfaceDX12, srWeaverDX12_)
SR_DEFINE_WEAVER_KNOBS(SRInterfaceOGL,  srWeaverOGL_)

#undef SR_DEFINE_WEAVER_KNOBS

//-------------------------------------------------------------------------
// Tracking — public surface
//-------------------------------------------------------------------------

HRESULT SimulatedReality::SRRequestTracking(unsigned int flags)
{
    // Too late once a context exists: its senses were registered before
    // initialize() and cannot be added afterwards. Say so rather than quietly
    // storing a request that will never take effect.
    if (srContext_ != nullptr)
        return E_NOT_VALID_STATE;

    srTrackingRequested_ = flags;
    return S_OK;
}

unsigned int SimulatedReality::SRActiveTracking()
{
    return srTrackingActive_;
}

HRESULT SimulatedReality::SRGetHead(SRHeadData* out)
{
    if (out == nullptr)
        return E_POINTER;

    if (srHeadCache_ == nullptr)
        return E_NOT_VALID_STATE;

    SR_head frame = {};
    if (!srHeadCache_->cache.load(frame))
        return E_PENDING;

    out->frameId = frame.frameId;
    out->time    = frame.time;
    CopyVec(frame.headPose.position,    out->position);
    CopyVec(frame.headPose.orientation, out->orientation);
    CopyVec(frame.eyes.left,            out->eyeLeft);
    CopyVec(frame.eyes.right,           out->eyeRight);
    CopyVec(frame.ears.left,            out->earLeft);
    CopyVec(frame.ears.right,           out->earRight);
    return S_OK;
}

HRESULT SimulatedReality::SRGetHeadPose(SRHeadPoseData* out)
{
    if (out == nullptr)
        return E_POINTER;

    if (srHeadPoseCache_ == nullptr)
        return E_NOT_VALID_STATE;

    SR_headPose frame = {};
    if (!srHeadPoseCache_->cache.load(frame))
        return E_PENDING;

    out->frameId = frame.frameId;
    out->time    = frame.time;
    CopyVec(frame.position,    out->position);
    CopyVec(frame.orientation, out->orientation);
    return S_OK;
}

HRESULT SimulatedReality::SRGetEyePair(SREyePairData* out)
{
    if (out == nullptr)
        return E_POINTER;

    if (srEyePairCache_ == nullptr)
        return E_NOT_VALID_STATE;

    SR_eyePair frame = {};
    if (!srEyePairCache_->cache.load(frame))
        return E_PENDING;

    out->frameId = frame.frameId;
    out->time    = frame.time;
    CopyVec(frame.left,  out->left);
    CopyVec(frame.right, out->right);
    return S_OK;
}
