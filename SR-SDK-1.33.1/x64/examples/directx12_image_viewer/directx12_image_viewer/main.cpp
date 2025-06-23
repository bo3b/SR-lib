/*!
 * Copyright (C) 2025 Leia, Inc.
 */

 // External dependencies
#include <thread>
#include <chrono>
#include <DirectXMath.h>
#include <shellscalingapi.h>

// Internal dependencies
#include "renderer.h"
#include "imageplane.h"
#include "shader.h"
#include "window.h"
#include "srcontainer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Simulated Reality includes
#include "d3dx12.h"
#include "sr/types.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/handtracker/handtracker.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/sense/system/systemsense.h"
#include "sr/sense/system/systemevent.h"
#include "sr/world/display/screen.h"
#include "sr/weaver/dx12weaver.h"
#include "sr/utility/exception.h"

unsigned char* convertTextureToCharArray(char* filePath, int& width, int& height, int& nrChannels) 
{
    unsigned char* data = NULL;

    //Load the image using the given filename.
    data = stbi_load(filePath, &width, &height, &nrChannels, 4);
    return data;
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

    // Create window object
    Window window("DirectXWindow", "Simulated Reality - Image Viewer 12 Demo");

    window.SetPositionOnTop(0, 0, systemResolutionWidth, systemResolutionHeight);

    // Create renderer object
    std::unique_ptr<Renderer> renderer = nullptr;

    //Can throw a runtime error if a DX12 compatible render adapter could not be found.
    try {
        renderer = std::make_unique<Renderer>(window, systemResolutionWidth, systemResolutionHeight, argv[1]);
    }
    catch (const std::exception& e) {
        std::cout << "Exception thrown: " << e.what();
        std::cout << "Press enter to exit.\n";
        std::cin.get();
        return 0;
    }

    TextureShader shader(*renderer);

    // Set up variables for storing width, height and nr of channels.
    int width, height, nrChannels;
    // Get char array from texture.
    unsigned char* data = convertTextureToCharArray(argv[1], width, height, nrChannels);

    // Allocate buffers for the image plane.
    // Load texture for the image plane. Copies the created texture to video memory.
    ImagePlane imagePlane(*renderer, data, width, height);

    // Texture is now loaded into video memory, free memory.
    stbi_image_free(data);

    // Wait for all assets to load
    renderer->WaitForFence();

    bool isSKeyDown = false;

    //! [SR Initialization]
    SRContainer srContainer;
    
    // The texture generated from the input image file does not change per frame,
    // so the texture loaded onto the GPU in the beginning is set as input buffer to PredictingDX12Weaver
    srContainer.createContext(renderer->GetDevice(), renderer->GetCommandAllocator(), renderer->GetCommandList(), renderer->GetCommandQueue(), imagePlane.GetTexture(), renderer->GetFirstBackBuffer(), window.GetHandle());
    if (!srContainer.getWeaver()->canWeave(systemResolutionWidth, systemResolutionHeight))
    {
        srContainer.destroyContext();
    }
    //! [SR Initialization]
    
    do 
    {
        // Check state of the "s" (0x53) key and switch lens if this key is pressed down (0x0800)".
        if (!(GetKeyState(0x53) & 0x0800)) 
        {
            isSKeyDown = false;
        }

        if (GetKeyState(0x53) & 0x0800) 
        {
            if (!isSKeyDown) 
            {
                // Destroy context if it is created, going into 2D-mode from 3D-mode
                if (srContainer.isContextCreated())
                {
                    srContainer.destroyContext();
                }
                else
                {
                    srContainer.createContext(renderer->GetDevice(), renderer->GetCommandAllocator(), renderer->GetCommandList(), renderer->GetCommandQueue(), imagePlane.GetTexture(), renderer->GetFirstBackBuffer(), window.GetHandle());

                    // Create context if it is destroyed and allows weaving, going into 3D-mode from 2D-mode
                    if (!srContainer.getWeaver()->canWeave(systemResolutionWidth, systemResolutionHeight))
                    {
                        srContainer.destroyContext();
                    }
                }

                // Lens and camera should be disabled if context is not created.
                isSKeyDown = true;
                char* result = (srContainer.isContextCreated()) ? "3D" : "2D";
                std::cout << "The screen is now in " << result << " mode.\n";
            }
        }
        window.Update();

        if (renderer->GetSize().x != window.GetSize().x || renderer->GetSize().y != window.GetSize().y)
        {
            renderer->Resize(window.GetSize());
        }

        renderer->GetCommandAllocator()->Reset();
        renderer->GetCommandList()->Reset(renderer->GetCommandAllocator(), nullptr);

        // Bind back buffer as render target
        D3D12_CPU_DESCRIPTOR_HANDLE BackBufferHandle = renderer->BindBackbufferAsTarget();

        // Clear back buffer
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        renderer->GetCommandList()->ClearRenderTargetView(BackBufferHandle, clearColor, 0, nullptr);

        // Determine viewport width for rendering to screen
        int renderWidth = 0;
        if (srContainer.isContextCreated())
        {
            renderWidth = window.GetSize().x;
        }
        else
        {
            // To see only the left part of the whole stereo image
            renderWidth = window.GetSize().x * 2;
        }
        
        // Set viewport for rendering to screen
        D3D12_VIEWPORT Viewport;
        Viewport.TopLeftX = 0;
        Viewport.TopLeftY = 0;
        Viewport.Width = renderWidth;
        Viewport.Height = window.GetSize().y;
        Viewport.MinDepth = 0;
        Viewport.MaxDepth = 1;

        D3D12_RECT ScissorRect;
        ScissorRect.top = 0;
        ScissorRect.left = 0;
        ScissorRect.right = renderWidth;
        ScissorRect.bottom = window.GetSize().y;

        renderer->GetCommandList()->RSSetViewports(1, &Viewport);
        renderer->GetCommandList()->RSSetScissorRects(1, &ScissorRect);

        if (srContainer.isContextCreated())
        {
            // Weave stereo image onto the back buffer using the input image texture 
            srContainer.getWeaver()->weave(renderer->GetSize().x, renderer->GetSize().y, 0, 0);
        }
        else
        {
            // Render the left part of the stereo image plane
            shader.Bind(renderer->GetCommandList());
            imagePlane.draw();
        }

        // Bind back buffer for presenting
        renderer->BindBackbufferForPresent();

        renderer->GetCommandList()->Close();

        renderer->ExecuteCommandList();

        // Present the frame.
        renderer->GetSwapChain()->Present(0, 0);

        // Wait for frame to finish
        renderer->WaitForFence();
    } while (
        !(GetKeyState(VK_ESCAPE) & 0x0800) && !window.ShouldClose()
        );

    // Wait for pending commands to finish before quitting
    renderer->WaitForFence();
}
