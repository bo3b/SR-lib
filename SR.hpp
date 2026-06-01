#pragma once

#include <d3d9.h>
#include <d3d11_1.h>

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

namespace SimulatedReality
{

//-------------------------------------------------------------------------
class SRInterfaceDX9
{
public:
    void SetInputTexture(IDirect3DTexture9* texture, bool isSRGB);
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

}  // namespace SimulatedReality
