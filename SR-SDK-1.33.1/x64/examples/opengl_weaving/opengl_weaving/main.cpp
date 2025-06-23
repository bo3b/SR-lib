/*!
 * Copyright (C) 2025 Leia, Inc.
 */

// External dependencies
#include <GL/glew.h>
#include <GLFW/glfw3.h>
GLFWwindow* window;
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include <chrono>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "glfw3native.h"

#include <shellscalingapi.h>
#undef near
#undef far

// Internal dependencies
#include "pyramid.h"
#include "shader.h"
#include "sr/weaver/glweaver.h"

// Simulated Reality includes
#include "sr/types.h"
#include "sr/sense/core/inputstream.h"
#include "sr/sense/handtracker/handtracker.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/sense/system/systemsense.h"
#include "sr/sense/system/systemevent.h"
#include "sr/world/display/display.h"
#include "sr/utility/exception.h"

#define WEAVING_ENABLED

using namespace std::chrono_literals;

// Flag to indicate that context is no longer valid
bool contextValid = false;

bool initializeSrObjects(); // forward declaration

// User implementation of the SystemEventListener interface
class SystemEventMonitor : public SR::SystemEventListener {
public:
    // Ensures SystemEventStream is cleaned up when Listener object is out of scope
    SR::InputStream<SR::SystemEventStream> stream;

    // The accept function can process the system event data as soon as it becomes available
    virtual void accept(const SR::SystemEvent& frame) override {
        switch (frame.eventType) {
        case SR_eventType::ContextInvalid:
        {
            std::cout
                << "ContextInvalid event received: "
                << frame.time << " "
                << frame.message << "\n";

            contextValid = false;

            std::thread([]() {
                initializeSrObjects();
                }).detach();

            return;
        }
            break;
        default:
            std::cout << "Unknown event type" << std::endl;
            break;
        }
    }

};

// My SR::EyePairListener that stores the last tracked eye positions
class MyEyes : public SR::EyePairListener {
private:
    SR::InputStream<SR::EyePairStream> stream;
public:
    glm::vec3 left, right;
    MyEyes(SR::EyeTracker* tracker) : left(-30, 0, 600), right(30, 0, 600) {
        // Open a stream between tracker and this class
        stream.set(tracker->openEyePairStream(this));
    }
    // Called by the tracker for each tracked eye pair
    virtual void accept(const SR_eyePair& eyePair) override
    {
        // Remember the eye positions
        left = glm::vec3(eyePair.left.x, eyePair.left.y, eyePair.left.z);
        right = glm::vec3(eyePair.right.x, eyePair.right.y, eyePair.right.z);
    }
};

// My SR::HandPoseListener that stores the last tracked index finger position
class MyFinger : public SR::HandPoseListener {
private:
    SR::InputStream<SR::HandPoseStream> stream;
public:
    glm::vec3 position;
    MyFinger(SR::HandTracker* tracker) : position(0, 0, 0) {
        // Open a stream between tracker and this class
        stream.set(tracker->openHandPoseStream(this));
    }
    // Called by the tracker for each tracked hand pose
    virtual void accept(const SR_handPose& handPose) override
    {
        // Remember the fingertip position
        position = glm::vec3(handPose.index.tip.x, handPose.index.tip.y, handPose.index.tip.z);
    }
};

// Create SRContext
SR::SRContext* context = nullptr;
SR::PredictingGLWeaver* weaver = nullptr;
std::mutex constructNewContextMutex;
MyEyes* eyes = nullptr;
SystemEventMonitor* listener = nullptr;

bool createSrContext() {
    if (context != nullptr) {
        delete context;
        context = nullptr;
    }
    while (context == nullptr && contextValid == false) {
        try {
            context = new SR::SRContext();
            return true;
        }
        catch (SR::ServerNotAvailableException e) {
            std::cout << "Server not available, trying again in 0.5 second" << std::endl;
            std::this_thread::sleep_for(500ms);
        }
    }
    return false;
}

bool initializeSrObjects() {
    std::lock_guard<std::mutex> lock(constructNewContextMutex);
    // weaver needs to be deleted before deleting SRContext as weaver is using the context, but this function does not reconstruct weaver. Construct weaver wherever is needed

    if (eyes != nullptr) {
        delete eyes;
        eyes = nullptr;
    }
    if (listener != nullptr) {
        delete listener;
        listener = nullptr;
    }

    while (weaver != nullptr) {
        //wait for it
        std::this_thread::sleep_for(10ms);
    }
    // constructing context
    if (createSrContext() == false) {
        return false;
    }
    // constructing EyePairListener
    eyes = new MyEyes(SR::EyeTracker::create(*context));

    SR::SystemSense* systemSense = SR::SystemSense::create(*context);
    listener = new SystemEventMonitor();
    // set systemEvent listener to the newly constructed systemsense
    listener->stream.set(systemSense->openSystemEventStream(listener));

    context->initialize();
    contextValid = true;
    return true;
}

glm::mat4 CalculateModel()
{
    const float size = 20.0; // Size of the object in mm
    using namespace std::chrono;
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    const int ms_per_revolution = 5000;
    float angle = float((2 * 3.1415 / ms_per_revolution) * (now % ms_per_revolution));
    
    glm::mat4 Model = glm::mat4();
    Model = scale(Model, glm::vec3(size, size, size)); //scale by size
    Model = translate(Model, glm::vec3(0, 1, 0));     //translate bottom face to origin
    Model = rotate(Model, angle, glm::vec3(0, 1, 0));  //rotate by angle

    return Model;
}

float screenWidth_mm = 697.0;
float screenHeight_mm = 394.0;

glm::mat4 CalculateProjection(glm::vec3 &eye)
{
    // Implementation of: Kooima, Robert. "Generalized perspective projection." J. Sch. Electron. Eng. Comput. Sci (2009).

    const float near = 1.0;
    const float far = 10000.0;
    const glm::vec3 pa(-screenWidth_mm / 2, screenHeight_mm / 2, 0);
    const glm::vec3 pb(screenWidth_mm / 2, screenHeight_mm / 2, 0);
    const glm::vec3 pc(-screenWidth_mm / 2, -screenHeight_mm / 2, 0);

    const glm::vec3 vr(1, 0, 0);
    const glm::vec3 vu(0, 1, 0);
    const glm::vec3 vn(0, 0, 1);

    // Compute the screen corner vectors.
    glm::vec3 va = pa - eye;
    glm::vec3 vb = pb - eye;
    glm::vec3 vc = pc - eye;

    // Find the distance from the eye to screen plane.
    float distance = -dot(va, vn);

    // Find the extent of the perpendicular projection.
    float l = dot(vr, va) * near / distance;
    float r = dot(vr, vb) * near / distance;
    float b = dot(vu, vc) * near / distance;
    float t = dot(vu, va) * near / distance;

    // Load the perpendicular projection
    glm::mat4 _frustum = glm::frustum(l, r, b, t, near, far);

    // Rotate the projection to be non-perpendicular
    glm::mat4 M = {
        vr[0], vu[0], vn[0],    0,
        vr[1], vu[1], vn[1],    0,
        vr[2], vu[2], vn[2],    0,
            0,     0,     0, 1.0,
    }; //(formatting is transposed from the paper!)

    // Move the apex of the frustum to the origin.
    glm::mat4 _translate = translate(glm::mat4(), -eye);

    // Combine
    return _frustum * M * _translate;
}

void RenderScene(GLuint MatrixID, const size_t renderWidth, const size_t renderHeight, glm::vec3 leftEye, glm::vec3 rightEye, Pyramid& pyramid) {
    //! [Drawing Scene]
    // Update Model to put object on the finger
    glm::mat4 View = glm::mat4();
    glm::mat4 Model = CalculateModel();

    // Set projection for left rendering
    glm::mat4 Projection = CalculateProjection(leftEye);
    glm::mat4 MVP = Projection * View * Model;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set viewport to the left half
    glViewport(0, 0, renderWidth, renderHeight);

    // Render the scene for the left eye
    pyramid.draw();

    // Set projection for right rendering
    Projection = CalculateProjection(rightEye);
    MVP = Projection * View * Model;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set viewport to the right half
    glViewport(renderWidth, 0, renderWidth, renderHeight);

    // Render the scene for the right eye
    pyramid.draw();
    //! [Drawing Scene]
}

// Returns true if function completed succesfully, returns false otherwise
bool ensureWindowFitsToMonitor() {
    // Get monitor that the window is currently located on.
    HMONITOR windowMonitor = MonitorFromWindow(glfwGetWin32Window(window), MONITOR_DEFAULTTONEAREST);

    // If no monitor handle was returned (no monitor may be attached), don't change window and return false
    if (!windowMonitor) {
        return false;
    }

    // Get monitor rectangle in the virtual screen
    MONITORINFO monitorInfo;
    ZeroMemory(&monitorInfo, sizeof(monitorInfo));
    monitorInfo.cbSize = sizeof(monitorInfo);
    GetMonitorInfoA(windowMonitor, &monitorInfo);

    // Get current window rectangle
    RECT windowRect;
    GetWindowRect(glfwGetWin32Window(window), &windowRect);

    // If the monitor rectangle is different from the current window rectangle...
    if (windowRect.left != monitorInfo.rcMonitor.left ||
        windowRect.right != monitorInfo.rcMonitor.right ||
        windowRect.top != monitorInfo.rcMonitor.top ||
        windowRect.bottom != monitorInfo.rcMonitor.bottom) {
        //...set the window rectangle to fit the monitor rectangle
        glfwSetWindowPos(window,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top
        );
        glfwSetWindowSize(window,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top
        );
    }

    // Return true, as function has completed succesfully
    return true;
}

bool windowFitsToMonitor = false;

void windowMoveCallback(GLFWwindow* window, int xPos, int yPos) {
    windowFitsToMonitor = false;
}

void monitorConfigurationCallback(GLFWmonitor* monitor, int event) {
    windowFitsToMonitor = false;
}

int main(void)
{
    // Ensure the application receives unscaled display metrics
    SetProcessDpiAwareness(PROCESS_DPI_AWARENESS::PROCESS_PER_MONITOR_DPI_AWARE);

    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    // Open a window and create its OpenGL context
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    window = glfwCreateWindow(mode->width, mode->height, "Simulated Reality - Cube demo", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    windowFitsToMonitor = true;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetWindowPosCallback(window, windowMoveCallback);
    glfwSetMonitorCallback(monitorConfigurationCallback);

    glClearColor(0.5f, 0.5f, 0.5f, 0.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    GLuint programID = loadBasicShaders();
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    //! [SR Initialization]
    initializeSrObjects();
    MyFinger finger(SR::HandTracker::create(*context));
    SR::Display* display = SR::Display::create(*context);
    //! [SR Initialization]

    glUseProgram(programID);

    // Get the screen width and height in millimeters
    int screenWidth_mm_i, screenHeight_mm_i;
    glfwGetMonitorPhysicalSize(monitor, &screenWidth_mm_i, &screenHeight_mm_i);
    screenWidth_mm = screenWidth_mm_i;
    screenHeight_mm = screenHeight_mm_i;

    //Allocate VAO for pyramid object
    Pyramid pyramid;

#ifdef WEAVING_ENABLED
    // Get default size for our views texture.
    const size_t renderWidth = display->getRecommendedViewsTextureWidth();
    const size_t renderHeight = display->getRecommendedViewsTextureHeight();
#else
    // Render to half of the resolution that the display is presenting itself as.
    // Devices like the SR Development Kit present themselves as a display with 7680 x 2160 resolution to allow for rendering at 3840 x 2160 for each view.
    // This is ideal for 8K displays like it.
    const size_t renderWidth = mode->width * 0.5f;
    const size_t renderHeight = mode->height;
#endif

#ifdef WEAVING_ENABLED
    //! [Construct weaver]
    weaver = new SR::PredictingGLWeaver(*context, renderWidth * 2, renderHeight, glfwGetWin32Window(window));

    context->initialize();
    //! [Construct weaver]
#endif

    bool lKeyPressed = false;

    do {
        
        // If the user presses the L key, toggle late latching.
        bool prevLKeyPressed = lKeyPressed;
        lKeyPressed = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        if (lKeyPressed && !prevLKeyPressed)
            weaver->enableLateLatching(!weaver->isLateLatchingEnabled());

#ifdef WEAVING_ENABLED
        //! [Bind framebuffer for weaving]
        // Draw to weaver with application specific shaders
        if (contextValid && weaver != nullptr) {
            glBindFramebuffer(GL_FRAMEBUFFER, weaver->getFrameBuffer());
        }
        else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0); //Start rendering to the display
        }
        //! [Bind framebuffer for weaving]
#endif
        glUseProgram(programID);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (contextValid) {
            RenderScene(MatrixID, renderWidth, renderHeight, eyes->left, eyes->right, pyramid);
        }
        else {
            RenderScene(MatrixID, renderWidth, renderHeight, { -30, 0, 600 }, { 30, 0, 600 }, pyramid);
        }

#ifdef WEAVING_ENABLED
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        //! [Render woven image]
        glBindFramebuffer(GL_FRAMEBUFFER, 0); //Start rendering to the display
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); does not need to be called because weaving overwrites each pixel on the backbuffer
        glViewport(0, 0, windowWidth, windowHeight);

        // For weaving, the center-point between both eyes is used.
        // It should be converted from millimeters to centimeters
        if (contextValid && weaver != nullptr) {
            weaver->weave((unsigned int)windowWidth, (unsigned int)windowHeight, 0, 0);
        }
        //! [Render woven image]
#endif

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (!windowFitsToMonitor) {
            windowFitsToMonitor = ensureWindowFitsToMonitor();
        }

#ifdef WEAVING_ENABLED
        // initialize weaver if it was deleted before
        if (contextValid && weaver == nullptr) {
            //! [Construct weaver]
            weaver = new SR::PredictingGLWeaver(*context, renderWidth * 2, renderHeight, glfwGetWin32Window(window));

            context->initialize();
            //! [Construct weaver]
        }

        // delete weaver if the SRContext is not valid
        if (contextValid == false && weaver != nullptr) {
            delete weaver;
            weaver = nullptr;
        }
#endif

    } while (
        glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS
        && glfwWindowShouldClose(window) == 0
        );

#ifdef WEAVING_ENABLED
    // Delete weaver GL resources before OpenGL context is deleted
    if (weaver != nullptr) {
        delete weaver;
        weaver = nullptr;
    }
#endif

    glDeleteProgram(programID);

    glfwTerminate();

    if (contextValid == false) {
        contextValid = true;
        std::this_thread::sleep_for(3000ms);
    }
}
