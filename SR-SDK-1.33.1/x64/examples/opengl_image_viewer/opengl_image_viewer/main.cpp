/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// External dependencies
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <shellscalingapi.h> // SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE)
#undef near
#undef far

// Image loader library (header-only)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Internal dependencies
#include "imageplane.h"
#include "shader.h"
#include "srcontainer.h"

// Simulated Reality includes
#include "sr/types.h"

#include <iostream>

// As there is no API yet for switching off the camera without destroying the SRContext object, we need to destroy the context when
// switching to 2D-mode. The lens will then automatically turn off as well and as the weaver is dependent on the context, we need to
// destroy and recreate the weaver together with the context. To make this easier and less error-prone, we created the SRContainer
// class. (defined in srcontainer.h)

void DrawStereoImage(const ImagePlane& imagePlane, GLuint shaderProgramID, GLFWwindow* window, SRContainer& container) {

    int windowWidth, windowHeight;
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

    // If we are rendering on an SR-device (which is a device that allows weaving) and 2D mode is not on, we will
    // render the stereo image into the input buffer of the weaver. This buffer is twice the width of the actual rendering
    // frame, the left half is considered to be the image for the left eye and the right half the image for the right
    // eye. The weaver will later use this input buffer to weave the left and right halves into one 3D image that is the
    // size of the final rendering frame.
    if (container.contextCreated()) {
        glViewport(0, 0, container.weaverInputWidth, container.weaverInputHeight);
        // Draw complete stereo image to weaver buffer
        glBindFramebuffer(GL_FRAMEBUFFER, container.weaver->getFrameBuffer());
    }
    // If we are not rendering to an SR-device or 2D mode is on, we will render the stereo image directly into the 
    // window back buffer but with twice the width, so that the image for the left eye is displayed in the rendering
    // frame and the image for the right eye is rendered outside the frame and is thus not visible.
    else {
        glViewport(0, 0, windowWidth * 2, windowHeight);
        // Draw complete stereo image to window back buffer (right half will be drawn outside of the frame)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Draw the image after we have set the buffer to draw to.
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    imagePlane.draw();

    // After we have drawn the image into the input buffer of the weaver, the weaver will weave the image and draw the
    // result into the window back buffer here. (If we are displaying only the left half of the stereo image, we wont
    // need to do this step.)
    if (container.contextCreated()) {
        // Select the window buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Set the viewport to the correct width
        glViewport(0, 0, windowWidth, windowHeight);

        // Weave the image and draw to the window back buffer (filling the entire buffer)
        container.weaver->weave((unsigned int)windowWidth, (unsigned int)windowHeight, 0, 0);
    }

    // Swap window buffers
    glfwSwapBuffers(window);
}

GLFWwindow* createGLFWWindow() {

    // Set window creation hints
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // Open a window and create its OpenGL context
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Simulated Reality - Cube demo", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        exit(1);
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    return window;
}

void makeOpenGLContextCurrent(GLFWwindow* window) {

    // Change the context to this GLFW window context
    glfwMakeContextCurrent(window);

    // (Re)load OpenGL function pointers after possible context change
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        exit(1);
    }

    // (Re)set OpenGL attributes after possible context change
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void loadImage(const char* file_path, unsigned char** imageData, GLsizei* imageWidth, GLsizei* imageHeight) {
    // Load image
    GLsizei imageNrChannels;
    *imageData = stbi_load(file_path, imageWidth, imageHeight, &imageNrChannels, 4);

    // Check image
    if (!(*imageData)) {
        std::cerr << "Image could not be read!" << std::endl;
        exit(1);
    }
}

void freeImage(unsigned char* imageData) {
    stbi_image_free(imageData);
}

int main(int argc, char* argv[]) {
    //! [Check if command line argument is provided]
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path to stereo image>" << std::endl;
        exit(1);
    }
    //! [Check if command line argument is provided]


    //! [Initialize GLFW and main window]
    // Ensure the application receives unscaled display metrics
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);

    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        exit(1);
    }

    GLFWwindow* window = createGLFWWindow();
    makeOpenGLContextCurrent(window);
    //! [Initialize GLFW and main window]
    

    //! [Load image and construct image plane]
    // Load image
    unsigned char* imageData;
    GLsizei imageWidth, imageHeight;
    loadImage(argv[1], &imageData, &imageWidth, &imageHeight);

    // Create image viewer object using image data (data is copied to GPU)
    // ImagePlane constructor modifies current OpenGL context
    ImagePlane imagePlane(imageData, imageWidth, imageHeight, GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA);

    // Memory allocated for the image on the CPU can be freed, as it is already loaded onto the GPU and we won't need to reload it.
    freeImage(imageData);
    //! [Load image and construct image plane]


    //! [Initialize container and check if device allows weaving]
    // Construct container object (initial context state is uncreated)
    SRContainer container;

    // Create SR context, check if device allows weaving and if not, destroy context as we wont be needing it.
    container.createContext(window);
    if (!container.weaver->canWeave()) {
        container.destroyContext();
    }
    //! [Initialize container and check if device allows weaving]


    //! [Generate shader program]
    GLuint shaderProgramID = createStaticTextureShader();
    //! [Generate shader program]


    //! [Main loop]
    bool pressingSKey = false;

    while (true) {
        glfwPollEvents();

        // Detect window close
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwWindowShouldClose(window)) {
            break;
        }

        // Detect S-key toggle
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            if (pressingSKey == false) {
                pressingSKey = true;

                // Destroy context if it is created, going into 2D-mode from 3D-mode
                if (container.contextCreated()) {
                    container.destroyContext();
                }
                // Create context if it is destroyed and allows weaving, going into 3D-mode from 2D-mode
                else {
                    container.createContext(window);
                    if (!container.weaver->canWeave()) {
                        container.destroyContext();
                    }
                }
            }
        }
        else {
            pressingSKey = false;
        }

        // Draw image on screen
        DrawStereoImage(imagePlane, shaderProgramID, window, container);
    }
    //! [Main loop]


    //! [Destroy used objects]
    // Destroy shader program, SR-context and image plane before GLFW is terminated.
    glDeleteProgram(shaderProgramID);
    if (container.contextCreated()) container.destroyContext();
    imagePlane.destroy();

    glfwTerminate();
    //! [Destroy used objects]
}
