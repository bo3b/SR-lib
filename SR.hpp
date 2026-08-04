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
//     SimulatedRealityOpenGL.dll;SimulatedRealityDisplays.dll;opencv_world343.dll
//   (32-bit builds: SimulatedRealityCore32.dll;SimulatedRealityDirectX32.dll;
//    SimulatedRealityOpenGL32.dll;SimulatedRealityDisplays32.dll;opencv_world343.dll)
// SimulatedRealityDisplays is what the lens-hint API below lives in; it is only
// needed by consumers that call SREnableLensHint()/SRDisableLensHint(), but
// listing it unconditionally is harmless (/DELAYLOAD on a DLL you don't import
// is a no-op warning, not an error).
// CMake consumers get the whole list applied for them by srlib_apply_delayload()
// — see CMakeLists.txt.
// Leave opengl32.dll as a normal import (it always ships with Windows).
// delayimp.lib is linked automatically once delay-loaded DLLs are specified.
// Delay-load cannot be baked into a static lib, so it must live in the
// consumer; our Create* functions then probe availability via LoadLibrary and
// return E_NOINTERFACE when the runtime is missing, instead of crashing.

namespace SimulatedReality
{

//-------------------------------------------------------------------------
class SRInterfaceDX9
{
public:
    void SetInputTexture(IDirect3DTexture9* texture, bool isSRGB, bool outputSRGB = true);
    void Delete();

    void Weave();
};
extern "C" HRESULT CreateSRInterfaceDX9(IDirect3DDevice9* device, HWND window, SRInterfaceDX9** ppReturnedSRInterfaceDX9);

//-------------------------------------------------------------------------
class SRInterfaceDX11
{
public:
    void SetInputTexture(ID3D11ShaderResourceView* texture);
    void Delete();

    void Weave();
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
extern "C" HRESULT SREnableLensHint();
extern "C" HRESULT SRDisableLensHint();

// *enabled receives whether the lens is currently on. Note this reflects the
// arbitrated state across all applications, not just our own preference.
extern "C" HRESULT SRIsLensHintEnabled(bool* enabled);

}  // namespace SimulatedReality
