/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

#include<memory>
#include "sr/management/srcontext.h"
#include "windows.h"
#include "sr/weaver/Weaver.h"

#ifdef WIN32
#   ifdef COMPILING_DLL_SimulatedRealityOpenGL
#     define DIMENCOSR_API __declspec(dllexport)
#   else
#     define DIMENCOSR_API __declspec(dllimport)
#   endif
#else
#   define DIMENCOSR_API
#endif

// GL types
typedef unsigned int GLuint;
typedef int GLsizei;

namespace SR {
    /*!
     * \brief class to be used to add Weaving to OpenGL based SR applications
     *
     * \ingroup API
     */
    class DIMENCOSR_API GLWeaverBase{
    protected:
        class Impl;
        /*!
         * Suppressing warning because if we don't want to export everything then solving the underlying problem requires modification of the API
         * Warning Description: 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
         * Candidate for deprecation
         */
#pragma warning(suppress: 4251)
        std::unique_ptr<Impl> pimpl;
    public:
        GLWeaverBase();
        ~GLWeaverBase();

        /*!
         * \brief Returns the buffer that will be used to create a weaved imaged. This buffer expects a side-by-side image.
         * \returns GLuint identifying a buffer generated with `glGenFramebuffers`
         *
         * The buffer identifier can be bound using `glBindFramebuffer` to start rendering to it.
         * Rendering to the buffer is identical to normal rendering and often starts with calling `glClear`.
         */
        GLuint getFrameBuffer();

        /*!
         * \brief Sets the buffer that will be used to create a weaved image.
         * \param frameBufferID ID of the frame buffer object used to render to the weaver input
         * \param renderedTextureID ID of the texture used as a buffer for the weaver input. This texture needs to be attached as a color attachment of the frame buffer object!
         */
        void setInputFrameBuffer(GLuint frameBufferID, GLuint renderedTextureID);

        /*!
         * \brief Sets the window handle of the application window. If the weaver was created using a deprecated constructor, setting window handle has no effect.
         * \param window Handle of the application window
         */
        void setWindowHandle(HWND handle);

        /*!
         * \deprecated
         * \brief Used to determine if software weaving is possible on this device
         * \returns bool indicating whether weaving can be done. Returns true when software weaving can be performed by GLWeaver. When false is returned, you can output side-by-side manually or let GLWeaver handle this.
         */
        [[deprecated("Use the canWeave(...) functions that accept parameters")]]
        bool canWeave();

        /*!
         * \brief Used to determine if software weaving is possible for certain size and visibility to the currently bound framebuffer
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * \throw std::runtime_error if the window handle (HWND) becomes invalid during the execution of canWeave
         * \returns bool indicating whether weaving can be done. Returns true when software weaving can be performed by GLWeaver. When false is returned, you can output side-by-side manually or let GLWeaver handle this.
         */
        bool canWeave(unsigned int width, unsigned int height);

        /*!
         * \brief Used to determine if software weaving is possible for certain size and visibility to the currently bound framebuffer
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * \param xOffset of the image to be rendered to the bound framebuffer
         * \param yOffset of the image to be rendered to the bound framebuffer
         * \throw std::runtime_error if the window handle (HWND) becomes invalid during the execution of canWeave
         * \returns bool indicating whether weaving can be done. Returns true when software weaving can be performed by GLWeaver. When false is returned, you can output side-by-side manually or let GLWeaver handle this.
         */
        bool canWeave(unsigned int width, unsigned int height, unsigned int xOffset, unsigned int yOffset);

        /*!
         * \brief Can be called to render a weaved image of a certain size to the currently bound framebuffer
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * Should often be preceeded by `glBindFramebuffer(GL_FRAMEBUFFER, 0)` to start rendering to the default framebuffer again.
         * The relative position of the user to the display in full-screen mode is equal to the direct output of the SR::EyeTracker class.
         */
        void weave(unsigned int width, unsigned int height);

        /*!
         * \brief Can be called to render a weaved image of a certain size to the currently bound framebuffer
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * \param xOffset of the image to be rendered to the bound framebuffer
         * \param yOffset of the image to be rendered to the bound framebuffer
         * Should often be preceeded by `glBindFramebuffer(GL_FRAMEBUFFER, 0)` to start rendering to the default framebuffer again.
         * The relative position of the user to the display in full-screen mode is equal to the direct output of the SR::EyeTracker class.
         */
        void weave(unsigned int width, unsigned int height, unsigned int xOffset, unsigned int yOffset);
    };

    class DIMENCOSR_API GLWeaver : public GLWeaverBase {
    public:
        /*!
         * \deprecated Calling delete on the returned pointer causes crashing, developers should use the constructor of GLWeaver directly
         *
         * \brief Constructs a class to be used for weaving an input image of a certain size
         * \param context to connect to, needs to be valid for the lifetime of the weaver object.
         * \param width of the side-by-side image to be weaved together
         * \param height of the side-by-side image to be weaved together
         * \throw std::exception if FrameBuffer did not initialize successfully, can be caused by using Opengl in parallel on another thread
         * \returns pointer to new GLWeaver instance
         */
        [[deprecated("use the constructor of GLWeaver directly")]]
        static GLWeaver* create(SR::SRContext& context, unsigned int width, unsigned int height);

        /*!
         * \brief Constructs a class to be used for weaving an input image of a certain size
         * \param context to connect to
         * \param width of the side-by-side image to be weaved together
         * \param height of the side-by-side image to be weaved together
         * \throw std::exception if FrameBuffer did not initialize successfully, can be caused by using Opengl in parallel on another thread
         */
        GLWeaver(SR::SRContext& context, unsigned int width, unsigned int height);

        /*!
         * \brief Handles proper destruction of all weaver related classes and buffers
         */
        ~GLWeaver();
    };

    class DIMENCOSR_API PredictingGLWeaver : public GLWeaverBase {
    public:
        /*!
         * \deprecated
         * \brief Constructs a class to be used for weaving an input image of a certain size
         * \param context to connect to, needs to be valid for the lifetime of the weaver object.
         * \param width of the side-by-side image to be weaved together
         * \param height of the side-by-side image to be weaved together
         * \throw std::exception if FrameBuffer did not initialize successfully can be caused by using Opengl in parallel on another thread
         */
        [[deprecated("Use non-deprecated constructors")]]
        PredictingGLWeaver(SR::SRContext& context, unsigned int width, unsigned int height);

        /*!
         * \brief Constructs a class to be used for weaving an input image of a certain size
         * \param context to connect to, needs to be valid for the lifetime of the weaver object.
         * \param width of the side-by-side image to be weaved together
         * \param height of the side-by-side image to be weaved together
         * \throw std::exception if FrameBuffer did not initialize successfully can be caused by using Opengl in parallel on another thread
         * \param window Handle of the application window
         */
        PredictingGLWeaver(SR::SRContext& context, unsigned int width, unsigned int height, HWND window);

        /*!
         * \brief Handles proper destruction of all weaver related classes and buffers
         */
        ~PredictingGLWeaver();

        /*!
         * \brief Set the latency to match the expected duration of the full rendering pipeline
         *
         * The eye positions should be predicted to the timepoint at which the frame is visible to the user
         * Internally the prediction is already taking care of all other latency, only the rendering pipeline latency is application dependent
         * A low latency app would have 1 framebuffer latency, so 16666 microseconds (the generated frame will be presented at next v-sync)
         * When using v-sync, the driver adds at least 1 buffer latency, and maybe the windows display manager also adds a buffer latency.
         * Typically, the latency is n * 1000*1000/framerate microseconds
         *
         * \param latency The latency from the moment when weave() is called until presenting the current frame to the user, in microseconds
         */
        void setLatency(uint64_t latency);

        /*!
         * \brief Set the latency to match the expected duration of the full rendering pipeline in number of frames.
         * The latency in time is calculated using these number of frames based on the refresh rate of the monitor that the application is running on, this will be dynamically updated when the window changes monitor.
         * For this it requires the weaver to be given a valid window handle of the running application.
         *
         * The eye positions should be predicted to the timepoint at which the frame is visible to the user
         * Internally the prediction is already taking care of all other latency, only the rendering pipeline latency is application dependent
         * A low latency app would have 1 framebuffer latency, the generated frame will be presented at next v-sync)
         * When using v-sync, the driver adds at least 1 buffer latency, and maybe the windows display manager also adds a buffer latency.
         *
         * \param latencyInFrames The expected number of frames before presenting the current generated frame to the user.
         */
        void setLatencyInFrames(uint64_t latencyInFrames);

    };
}

#undef DIMENCOSR_API
