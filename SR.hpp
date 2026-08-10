#pragma once

#include <d3d9.h>
#include <d3d11_1.h>
#include <d3d12.h>

// This provides a simple interface to any Simulated Reality hardware.
//
// Designed to follow the COM model of DX9 and DX11, as opposed to a
// more C++ object style. Follow these steps to use the interface:
//
//  1) Call CreateSRInterfaceDX9() to create an SRInterfaceDX9 object.
//     SRContext and Weaver will be initialized.
//  2) Call SetInputTexture for surface you will use for stereo output
//  3) Draw loop:
//    1) Render your side-by-side stereo image into input texture.
//    2) Bind your output render target / backbuffer.
//    3) Call Weave() to weave the stereo into the bound target.
//    4) Call Present() to display stereo.
//  4) On exit, call Delete()
//
// NOTE (SDK 1.34.10): the weaver no longer owns the intermediate buffer.
// You render into your own texture and pass it in via SetInputTexture();
// Weave() writes into whatever render target is currently bound.
// Anti-crosstalk (Dynamic ACT) is applied automatically by the weaver.
//
// DELAY-LOAD REQUIREMENT (important for whoever links this lib):
// This is a static library, and the SR import libraries are merged into it,
// so linking SR-mt.lib (or -mtd/-md/-mdd) gives your module a load-time (hard)
// dependency on the SR runtime DLLs. If those DLLs are absent the process will
// fail to launch BEFORE any of our code runs, defeating the LoadLibrary check
// we do internally. To keep the dependency soft, the FINAL module that links
// this lib (e.g. the host EXE/DLL) must delay-load the SR DLLs in its linker
// settings:
//   Linker > Input > Delay Loaded Dlls:
//     SimulatedRealityCore.dll;SimulatedRealityDirectX.dll;
//     SimulatedRealityOpenGL.dll;SimulatedRealityDisplays.dll;
//     SimulatedRealityFaceTrackers.dll;opencv_world343.dll
//   (32-bit builds: SimulatedRealityCore32.dll;SimulatedRealityDirectX32.dll;
//    SimulatedRealityOpenGL32.dll;SimulatedRealityDisplays32.dll;
//    SimulatedRealityFaceTrackers32.dll;opencv_world343.dll)
// SimulatedRealityDisplays holds the lens-hint API and SimulatedRealityFaceTrackers
// holds the head/eye trackers, so those two are only pulled in by consumers that
// use them — but listing them unconditionally is harmless (/DELAYLOAD on a DLL
// you don't import is a no-op warning, not an error).
// Don't hand-copy this list if you can avoid it — it grows as SR.cpp starts
// using new SR components, and a consumer that misses one ships a module that
// won't load without the runtime. CMake consumers get the whole list applied
// for them by srlib_apply_delayload() — see CMakeLists.txt. MSBuild consumers
// import SR-lib.props, which does the same thing for both platforms.
// Leave opengl32.dll as a normal import (it always ships with Windows).
// delayimp.lib is linked automatically once delay-loaded DLLs are specified.
// Delay-load cannot be baked into a static lib, so it must live in the
// consumer; our Create* functions then probe availability via LoadLibrary and
// return E_NOINTERFACE when the runtime is missing, instead of crashing.

namespace SimulatedReality
{

//-------------------------------------------------------------------------
// Weaver tuning, common to every backend
//-------------------------------------------------------------------------
// Each SRInterface* below carries the same five calls, because they all sit on
// the SDK's shared weaver base. Create* applies sensible defaults
// (SetLatencyInFrames(1), EnableLateLatching(true)) so you only need these if
// those don't suit you — with one exception, SetShaderSRGBConversion, which has
// no default that is right for everyone:
//
//   SetShaderSRGBConversion(read, write)
//     Whether the weaver applies gamma conversion sampling your input texture
//     and writing its output. Get this wrong and the image is over- or
//     under-saturated rather than broken, so it is easy to ship by accident.
//     There is no universally correct setting — it depends on whether your
//     input and output carry sRGB-encoded or linear values:
//       (true, true)   input is sRGB-encoded, output surface is _UNORM/linear
//                      — the weaver decodes on read and re-encodes on write.
//       (false, false) both sides already hold the values you want, e.g. your
//                      compose shader already did the sRGB encode, or you bound
//                      _SRGB views and the hardware is doing it. Letting the
//                      weaver convert as well double-applies gamma.
//     Shipping integrations genuinely differ here, so test it against your own
//     formats rather than copying a value.
//
//   SetLatencyInFrames / SetLatency
//     How far ahead the weaver predicts eye position. Frames is the easy one;
//     SetLatency takes microseconds if you know your actual pipeline depth.
//
//   EnableLateLatching
//     Re-samples tracking as late as possible before the weave. Can reduce
//     crosstalk. On by default.
//
//   GetPredictedEyePositions
//     The predicted left/right eye positions the weaver is about to weave with,
//     in millimetres. This is the cheapest possible eye-position source — it
//     needs no tracker, no listener and no SRRequestTracking() opt-in, because
//     the weaver is already computing it. If all you want is to drive a virtual
//     stereo camera, prefer this over the tracking API further down. Returns
//     false if no weaver exists yet.

//-------------------------------------------------------------------------
class SRInterfaceDX9
{
public:
    void SetInputTexture(IDirect3DTexture9* texture, bool isSRGB, bool outputSRGB = true);
    void Delete();

    void Weave();

    // Shared weaver tuning — see the notes above the SRInterface classes.
    void SetShaderSRGBConversion(bool read, bool write);
    void SetLatencyInFrames(unsigned long long frames);
    void SetLatency(unsigned long long microseconds);
    void EnableLateLatching(bool enable);
    bool GetPredictedEyePositions(float left[3], float right[3]);
};
extern "C" HRESULT CreateSRInterfaceDX9(IDirect3DDevice9* device, HWND window, SRInterfaceDX9** ppReturnedSRInterfaceDX9);

//-------------------------------------------------------------------------
class SRInterfaceDX11
{
public:
    void SetInputTexture(ID3D11ShaderResourceView* texture);
    void Delete();

    void Weave();

    // Shared weaver tuning — see the notes above the SRInterface classes.
    void SetShaderSRGBConversion(bool read, bool write);
    void SetLatencyInFrames(unsigned long long frames);
    void SetLatency(unsigned long long microseconds);
    void EnableLateLatching(bool enable);
    bool GetPredictedEyePositions(float left[3], float right[3]);
};
extern "C" HRESULT CreateSRInterfaceDX11(ID3D11DeviceContext* context, HWND window, SRInterfaceDX11** ppReturnedSRInterfaceDX11);

//-------------------------------------------------------------------------
// DX12 is the odd one out. DX9/DX11/OGL weavers hold their own device or
// context and can weave from a single stored input texture, so those interfaces
// are "set the texture once, Weave() every frame". DX12 has no implicit
// context: the weaver records into a command list you supply, and it rasterizes
// against a viewport/scissor. Both change per frame, hence the extra setters.
//
// Per-frame order is: SetCommandList -> SetViewport/SetScissorRect -> Weave(),
// or the single Weave(cmd, viewport, scissor) overload that does all three.
//
// ⚠ Two things this wrapper CANNOT do for you:
//
//  1) Reset the command list's own rasterizer viewport. D3D12 rasterizes
//     against whatever RSSetViewports() last set ON THE COMMAND LIST — not
//     against what we pass to the weaver's setViewport(). If your compose pass
//     left the command-list viewport at the (wider) SbS intermediate size, the
//     weave renders at that size and the swap chain shows only its left
//     portion. Call RSSetViewports/RSSetScissorRects with the swap-chain
//     dimensions yourself, on the command list, immediately before Weave().
//
//  2) Transition the input resource. It must already be in
//     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE (or the generic read state)
//     and the destination render target must already be bound.
class SRInterfaceDX12
{
public:
    // Dimensions and format are read straight off the resource desc, so the
    // weaver can never be told a size the texture doesn't actually have.
    // Pass the FULL combined side-by-side texture (2W x H for a W-per-eye
    // pair) — the DX12 weaver samples exactly width x height texels and
    // expects L in the left half, R in the right.
    void SetInputTexture(ID3D12Resource* texture);

    // Must match the bound render target's actual format. A mismatch (e.g.
    // hardcoding R8G8B8A8_UNORM against a B8G8R8A8_UNORM swap chain) weaves
    // with visibly wrong colors rather than failing.
    void SetOutputFormat(DXGI_FORMAT format);

    void SetCommandList(ID3D12GraphicsCommandList* commandList);
    void SetViewport(const D3D12_VIEWPORT& viewport);
    void SetScissorRect(const D3D12_RECT& scissorRect);

    void Delete();

    void Weave();
    void Weave(ID3D12GraphicsCommandList* commandList, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect);

    // Shared weaver tuning — see the notes above the SRInterface classes.
    void SetShaderSRGBConversion(bool read, bool write);
    void SetLatencyInFrames(unsigned long long frames);
    void SetLatency(unsigned long long microseconds);
    void EnableLateLatching(bool enable);
    bool GetPredictedEyePositions(float left[3], float right[3]);
};
extern "C" HRESULT CreateSRInterfaceDX12(ID3D12Device* device, HWND window, SRInterfaceDX12** ppReturnedSRInterfaceDX12);

//-------------------------------------------------------------------------
// OpenGL has no lightweight types-only header, so (like the SR SDK's own
// glweaver.h, GLEW, glad, etc.) we alias the GL handle types we expose.
// These match <GL/gl.h> exactly, so they are harmless redefinitions if a
// real GL header is also included, and they avoid dragging GL into DX-only
// consumers.
typedef unsigned int GLuint;

class SRInterfaceOGL
{
public:
    void SetInputTexture(GLuint texture);
    void Delete();

    void Weave();

    // Shared weaver tuning — see the notes above the SRInterface classes.
    void SetShaderSRGBConversion(bool read, bool write);
    void SetLatencyInFrames(unsigned long long frames);
    void SetLatency(unsigned long long microseconds);
    void EnableLateLatching(bool enable);
    bool GetPredictedEyePositions(float left[3], float right[3]);
};
extern "C" HRESULT CreateSRInterfaceOGL(HWND window, SRInterfaceOGL** ppReturnedSRInterfaceOGL);

//-------------------------------------------------------------------------
// Switchable lens hint
//-------------------------------------------------------------------------
// Some SR panels have a switchable lenticular lens: with the lens off the
// display is an ordinary sharp 2D monitor, with it on it is autostereoscopic.
// The hint is a *preference*, not a command — the SR service arbitrates
// between all connected applications, so the lens stays on while any app wants
// it on.
//
// Typical use is to follow your window's focus/visibility rather than your
// weaving state: enable when your stereo window comes to the front, disable
// when it loses focus or before any content is on screen, so the user gets a
// normal 2D desktop instead of a lenticular blur over other windows.
//
// These are context-scoped, not weaver-scoped, so they are free functions
// rather than methods — they apply no matter which SRInterface* you created,
// and are shared if you somehow created more than one.
//
// The hint object is created lazily on first use and is owned by the SRContext
// (it is released when the last interface is Delete()d; never delete it
// yourself). All three return E_NOINTERFACE if no SRContext exists yet (call a
// CreateSRInterface* first) or if the connected display has no switchable lens.
//
// These two are built to be driven straight from per-frame state ("am I weaving
// this frame?"), so calling them every frame is fine and intended:
//
//   S_OK          the preference CHANGED and was sent to the service
//   S_FALSE       already in that state; nothing was sent
//   E_NOINTERFACE no context, or this display has no switchable lens (latched
//                 after the first attempt, so it costs one failed create, not
//                 one per frame)
//   E_FAIL        the SDK threw
//
// Both S_OK and S_FALSE are SUCCEEDED(). Branch on `== S_OK` when you want to
// log or react to the transition only — that is what the distinction is for,
// and it saves you shadowing the state on your side just to detect a change.
extern "C" HRESULT SREnableLensHint();
extern "C" HRESULT SRDisableLensHint();


// *enabled receives whether the lens is currently on. Note this reflects the
// arbitrated state across all applications, not just our own preference.
extern "C" HRESULT SRIsLensHintEnabled(bool* enabled);

//-------------------------------------------------------------------------
// Head and eye tracking
//-------------------------------------------------------------------------
// The SDK delivers tracking by callback: you subclass a listener, open a
// stream, and get accept() calls on an SR thread. That means every consumer
// writes the same listener + stream + mutex boilerplate before it can read a
// head position. This wraps that once — an internal listener caches the most
// recent frame under a lock, and you poll it whenever suits your render loop.
//
// ORDERING — this is the part that bites. SR senses must be created BEFORE
// SRContext::initialize(), and our CreateSRInterface* functions call
// initialize() at the end of their work. So tracking cannot be switched on
// after the fact the way the lens hint can:
//
//     SRRequestTracking(SR_TRACK_HEAD);          // FIRST
//     CreateSRInterfaceDX12(device, hwnd, &sr);  // creates senses, initializes
//     ...
//     SRHeadData head;
//     if (SUCCEEDED(SRGetHead(&head))) { ... }   // poll per frame
//
// Calling SRRequestTracking after a CreateSRInterface* returns E_NOT_VALID_STATE
// and changes nothing. Delete()ing the last interface tears the senses down and
// re-arms the request, so you can pick different flags on a later create.
//
// If you only want eye positions to drive a stereo camera, you probably don't
// need any of this — GetPredictedEyePositions() on your interface gives you the
// weaver's own prediction with no opt-in and no tracker at all.
//
// All coordinates are in MILLIMETRES, in the SR world space whose origin is the
// centre of the display. Orientation is radians, (x, y, z) = (pitch, yaw, roll),
// positive rotations clockwise from the user's perspective.

// Mirrors the SDK's SR_point3d without dragging its headers into this one.
struct SRVec3
{
    double x, y, z;
};

// Mirrors SR_headPose (SR::HeadPoseTracker).
struct SRHeadPoseData
{
    unsigned long long frameId;      // autoincrement frame number
    unsigned long long time;         // capture time, microseconds since epoch
    SRVec3             position;     // head position, mm
    SRVec3             orientation;  // pitch, yaw, roll in radians
};

// Mirrors SR_head (SR::HeadTracker) — the richer feed: pose plus the derived
// eye and ear positions the SDK computes from it.
struct SRHeadData
{
    unsigned long long frameId;
    unsigned long long time;
    SRVec3             position;
    SRVec3             orientation;
    SRVec3             eyeLeft;
    SRVec3             eyeRight;
    SRVec3             earLeft;
    SRVec3             earRight;
};

// Mirrors SR_eyePair (SR::EyeTracker).
struct SREyePairData
{
    unsigned long long frameId;
    unsigned long long time;
    SRVec3             left;
    SRVec3             right;
};

// Which senses to create. Combine with |. Each maps to one SDK tracker:
//
//   SR_TRACK_HEAD       SR::HeadTracker      -> SRGetHead()
//                       Pose plus eye and ear positions in one frame. This is
//                       the one to pick if you want eyes AND head orientation,
//                       since it carries both without running two senses.
//   SR_TRACK_HEAD_POSE  SR::HeadPoseTracker  -> SRGetHeadPose()
//                       Pose only. Cheaper if that is genuinely all you need.
//   SR_TRACK_EYES       SR::EyeTracker       -> SRGetEyePair()
//                       Filtered eye positions, smoothed by the SDK.
//   SR_TRACK_EYES_RAW   SR::EyeTracker (raw) -> SRGetEyePair()
//                       Unfiltered. Use when you apply your own filtering and
//                       the SDK's smoothing would fight it. Mutually exclusive
//                       with SR_TRACK_EYES; if both are set, RAW wins.
enum SRTrackingFlags : unsigned int
{
    SR_TRACK_NONE      = 0u,
    SR_TRACK_HEAD      = 1u << 0,
    SR_TRACK_HEAD_POSE = 1u << 1,
    SR_TRACK_EYES      = 1u << 2,
    SR_TRACK_EYES_RAW  = 1u << 3,
};

// Request senses. MUST be called before CreateSRInterface*; see ORDERING above.
// Returns E_NOT_VALID_STATE if an interface already exists.
extern "C" HRESULT SRRequestTracking(unsigned int flags);

// Which senses actually started. A tracker whose create() failed — no camera,
// unsupported hardware — is dropped rather than failing the whole interface
// creation, so this can be a subset of what you requested. Check it once after
// CreateSRInterface* if you need to know whether to fall back.
extern "C" unsigned int SRActiveTracking();

// Poll the most recent frame. Each returns:
//   S_OK               *out holds a frame
//   E_PENDING          sense is running but no frame has arrived yet
//   E_NOT_VALID_STATE  that sense was never requested or failed to start
//   E_POINTER          out is null
// Safe to call from your render thread; the SDK's callback thread writes under
// the same lock.
extern "C" HRESULT SRGetHead(SRHeadData* out);
extern "C" HRESULT SRGetHeadPose(SRHeadPoseData* out);
extern "C" HRESULT SRGetEyePair(SREyePairData* out);

}  // namespace SimulatedReality
