/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// External dependencies
#include <DirectXMath.h>
#include <shellscalingapi.h>

// Internal dependencies
#include "imageplane.h"
#include "shader.h"
#include "renderer.h"
#include "window.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "SRContainer.h"

// Create Window object
Window window("DirectXWindow", "Simulated Reality - Image demo");

bool isSKeyDown = false;

struct TransformationStruct
{
    DirectX::XMMATRIX Transformation;
};
TransformationStruct TransformationData;

void RenderScene(Renderer& renderer, const int renderWidth, const int renderHeight, ImagePlane& imagePlane) {
    //! [Drawing Scene]
    // Set viewport for the image plane. Width is doubled so the rightmost stereo image can be weaved by the SRWeaver if SR is enabled.
    D3DVIEWPORT9 Viewport;
    Viewport.X = 0;
    Viewport.Y = 0;
    Viewport.Width = renderWidth * 2;
    Viewport.Height = renderHeight;
    Viewport.MinZ = 0.0f;
    Viewport.MaxZ = 1.0f;
    renderer.GetDevice()->SetViewport(&Viewport);

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

D3DVIEWPORT9 setUpViewPort(Window window) {
    D3DVIEWPORT9 Viewport;
    Viewport.X = 0;
    Viewport.Y = 0;
    Viewport.Width = window.GetSize().x;
    Viewport.Height = window.GetSize().y;
    Viewport.MinZ = 0.0f;
    Viewport.MaxZ = 1.0f;
    
    return Viewport;
}

D3DVIEWPORT9 renderImagePlane(IDirect3DSurface9* buffer, Renderer& renderer, TextureShader& shader, int renderWidth, int renderHeight, Window& window, ImagePlane& imagePlane) {
    IDirect3DSurface9* View = buffer;

    renderer.GetDevice()->SetRenderTarget(0, View);
    const D3DCOLOR clearColor = D3DCOLOR(0xFF000000);
    renderer.GetDevice()->Clear(0, NULL, D3DCLEAR_TARGET, clearColor, 1.0f, 0);

    shader.Bind();

    // Render the image plane
    RenderScene(renderer, renderWidth, renderHeight, imagePlane);

    View = renderer.GetBackbuffer();
    renderer.GetDevice()->SetRenderTarget(0, View);
    D3DVIEWPORT9 Viewport = setUpViewPort(window);
    renderer.GetDevice()->SetViewport(&Viewport);

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
    if (!container.weaver->canWeave(width, height)) {
        container.destroyContext();
    }
    //! [SR Initialization]

    // Set shader constant for the matrix
    renderer.GetDevice()->SetVertexShaderConstantF(0, (const float*) &TransformationData, sizeof(TransformationData) / sizeof(float[4]));

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
                    if (!container.weaver->canWeave(width, height)) {
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
            imagePlane.invalidateDeviceObjects();
            if (container.isContextCreated()) {
                container.weaver->invalidateDeviceObjects();
            }
            renderer.Resize(window.GetSize());
            if (container.isContextCreated()) {
                container.weaver->restoreDeviceObjects();
            }
            imagePlane.restoreDeviceObjects();
        }

        renderer.GetDevice()->BeginScene();

        // Split between weaver render and raw render depending on if SRContext is created or not.
        if (container.isContextCreated()) {
            D3DVIEWPORT9 Viewport = renderImagePlane(container.weaver->getFrameBuffer(), renderer, shader, container.weaverInputWidth, container.weaverInputHeight, window, imagePlane);

            // Weave rendered image.
            container.weaver->weave(Viewport.Width, Viewport.Height, Viewport.X, Viewport.Y);
        }
        else {
            renderImagePlane(renderer.GetBackbuffer(), renderer, shader, window.GetSize().x, window.GetSize().y, window, imagePlane);
        }

        renderer.GetDevice()->EndScene();

        // Swap buffers
        renderer.GetDevice()->Present(NULL, NULL, NULL, NULL);
    } while (!(GetKeyState(VK_ESCAPE) & 0x0800) && !window.ShouldClose());
}
