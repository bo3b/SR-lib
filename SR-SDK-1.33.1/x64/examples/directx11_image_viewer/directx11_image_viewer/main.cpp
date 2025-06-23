/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// External dependencies
#include <DirectXMath.h>
#include <shellscalingapi.h>
#include<iostream>
#include<conio.h>
#include <future>

// Internal dependencies
#include "imageplane.h"
#include "shader.h"
#include "renderer.h"
#include "window.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "SRContainer.h"

// Simulated Reality includes
#include "sr/types.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/handtracker/handtracker.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/world/display/screen.h"
#include "sr/weaver/dx11weaver.h"

HWND WindowHandle;
bool isSKeyDown = false;

struct TransformationStruct
{
    DirectX::XMMATRIX Transformation;
};
TransformationStruct TransformationData;
ID3D11Buffer* TransformationBuffer = nullptr;

void RenderScene(Renderer& renderer, const size_t renderWidth, const size_t renderHeight, ImagePlane& imagePlane) {
    //! [Drawing Scene]
    // Set viewport for the image plane. Width is doubled so the rightmost stereo image can be weaved by the SRWeaver if SR is enabled.
    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = renderWidth*2;
    Viewport.Height = renderHeight;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    renderer.GetContext()->RSSetViewports(1, &Viewport);

    // This method returns a single image plane (triangle).
    imagePlane.draw();
    //! [Drawing Scene]
}

std::string getString()
{
    std::string input;
    // Change this to non-command-line key getter.
    std::getline(std::cin, input);
    return input;
}
    
unsigned char* convertTextureToCharArray(char* filePath, int& width, int& height, int& nrChannels){
    unsigned char* data = NULL;

    // Load the image using the given filename.
    data = stbi_load(filePath, &width, &height, &nrChannels, 4);
    return data;
}

D3D11_VIEWPORT setUpViewPort(Window window) {
    D3D11_VIEWPORT Viewport;
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = window.GetSize().x;
    Viewport.Height = window.GetSize().y;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    
    return Viewport;
}

D3D11_VIEWPORT renderImagePlane(ID3D11RenderTargetView* buffer, Renderer& renderer, TextureShader& shader, int renderWidth, int renderHeight, Window& window, ImagePlane& imagePlane) {
    ID3D11RenderTargetView* View = buffer;

    float Color[4] = { 0, 0, 0, 1 };
    renderer.GetContext()->ClearRenderTargetView(View, Color);
    renderer.GetContext()->OMSetRenderTargets(1, &View, nullptr);

    shader.Bind();

    // Render the image plane
    RenderScene(renderer, renderWidth, renderHeight, imagePlane);

    View = renderer.GetBackbuffer();
    renderer.GetContext()->OMSetRenderTargets(1, &View, nullptr);
    D3D11_VIEWPORT Viewport = setUpViewPort(window);
    renderer.GetContext()->RSSetViewports(1, &Viewport);

    return Viewport;
}

int main(int argc, char* argv[])
{   
    if (argc != 2) {
        std::cout << "Invalid amount of arguments.\n Please run the program with one command line argument containing a path on disk to an image.";
        return 1;
    }

    // Ensure the application receives unscaled display metrics
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);

    size_t systemResolutionWidth = GetSystemMetrics(SM_CXSCREEN);
    size_t systemResolutionHeight = GetSystemMetrics(SM_CYSCREEN);

    Window window("DirectXWindow", "Simulated Reality - DirectX Image Demo");

    window.SetPositionOnTop(0, 0, systemResolutionWidth, systemResolutionHeight);

    Renderer renderer(window);
    TextureShader shader(renderer);

    // Set up some ints for storing width, height and nr of channels.
    int width, height, nrChannels;
    // Get char array from texture.
    unsigned char* data = convertTextureToCharArray(argv[1], width, height, nrChannels);

    // Allocate buffers for the image plane.
    // Load texture for the image plane. Copies the created texture to video memory.
    ImagePlane imagePlane(renderer, data, width, height);

    // Texture is now loaded into video memory, free memory.
    stbi_image_free(data);

    //! [SR Initialization]
    // Initialize SR
    SRContainer container;
    container.createContext(renderer, window);
    if (!container.weaver->canWeave()) {
        container.destroyContext();
    }
    //! [SR Initialization]

    // Create constant buffer for the matrix
    D3D11_BUFFER_DESC Desc = { };
    Desc.ByteWidth = sizeof(TransformationStruct);
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA Resource = { };
    Resource.pSysMem = &TransformationData;

    renderer.GetDevice()->CreateBuffer(&Desc, &Resource, &TransformationBuffer);
    TransformationData.Transformation = DirectX::XMMatrixIdentity();
    renderer.GetContext()->UpdateSubresource(TransformationBuffer, 0, nullptr, &TransformationData, 0, 0);
    renderer.GetContext()->VSSetConstantBuffers(0, 1, &TransformationBuffer);

    do {
        // Check state of the "s" (0x53) key and switch lens if this key is pressed down (0x0800)".
        if (!(GetKeyState(0x53) & 0x0800)) {
            isSKeyDown = false;
        }

        if (GetKeyState(0x53) & 0x0800) {
            if (!isSKeyDown) {
                // We create a new SRContext if it's not present, otherwise we destroy it to restore it to its default state.
                if (container.isContextCreated()) {
                    container.destroyContext();
                }
                else {
                    // Create context if it is destroyed and allows weaving, going into 3D-mode from 2D-mode
                    container.createContext(renderer, window);
                    if (!container.weaver->canWeave()) {
                        container.destroyContext();
                    }
                }

                // Lens and camera should be disabled if context is not created.
                isSKeyDown = true;
                char* result = (container.isContextCreated()) ? "3D" : "2D";
                std::cout << "The screen is now in " << result << " mode.\n";
            }
        }

        window.Update();

        if (renderer.GetSize().x != window.GetSize().x || renderer.GetSize().y != window.GetSize().y)
        {
            // Update the swap chain size according to the new window size.
            renderer.Resize(window.GetSize());
        }

        // Split between weaver render and raw render depending on if SRContext is created or not.
        if (container.isContextCreated()) {
            D3D11_VIEWPORT Viewport = renderImagePlane(container.weaver->getFrameBuffer(), renderer, shader, container.weaverInputWidth, container.weaverInputHeight, window, imagePlane);

            // Weave rendered image.
            container.weaver->weave(Viewport.Width, Viewport.Height, Viewport.TopLeftX, Viewport.TopLeftY);
        }
        else {
            renderImagePlane(renderer.GetBackbuffer(), renderer, shader, window.GetSize().x, window.GetSize().y, window, imagePlane);
        }

        // Swap buffers
        renderer.GetSwapChain()->Present(1, 0);
    } while (!(GetKeyState(VK_ESCAPE) & 0x0800) && !window.ShouldClose());

    if(TransformationBuffer != nullptr)
    {
        TransformationBuffer->Release();
        TransformationBuffer = nullptr;
    }
}
