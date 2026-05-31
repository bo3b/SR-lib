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
//  2) Draw loop:
//    1) Call GetSRRenderSurface() to get the SR Surface/RenderTarget.
//    2) Draw or StretchRect() final output into that Surface/RenderTarget.
//    3) Call PerformWeave() to have the stereo drawn to backbuffer.
//    4) Call Present() to display stereo.
//  3) On exit, call Release()

namespace SimulatedReality
{

class SRInterfaceDX9
{
public:
    void Release();

    void GetSRRenderSurface(IDirect3DSurface9** renderTarget);
    void PerformWeave();
};
extern "C" HRESULT CreateSRInterfaceDX9(IDirect3DDevice9* device, unsigned int width, unsigned int height, HWND window, SRInterfaceDX9** ppReturnedSRInterfaceDX9);


class SRInterfaceDX11
{
public:
    void Release();

    void GetSRRenderSurface(ID3D11RenderTargetView** renderTarget);
    void PerformWeave();
};
extern "C" HRESULT CreateSRInterfaceDX11(ID3D11Device1* device1, ID3D11DeviceContext1* context1, unsigned int width, unsigned int height, HWND window, SRInterfaceDX11** ppReturnedSRInterfaceDX11);


typedef unsigned int GLuint;  // Standard interface for OpenGL

class SRInterfaceOGL
{
public:
    void Release();

    void GetSRRenderSurface(GLuint* frameBuffer);
    void PerformWeave();
};

extern "C" HRESULT CreateSRInterfaceOGL(unsigned int width, unsigned int height, HWND window, SRInterfaceOGL** ppReturnedSRInterfaceOGL);

}  // namespace SimulatedReality