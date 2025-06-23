/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once
#include <memory>
#include <d3d9.h>
#include "sr/management/srcontext.h"
#include "sr/weaver/Weaver.h"

#ifdef WIN32
#   ifdef COMPILING_DLL_SimulatedRealityDirectX
#     define DIMENCOSR_API __declspec(dllexport)
#   else
#     define DIMENCOSR_API __declspec(dllimport)
#   endif
#else
#   define DIMENCOSR_API
#endif

/*!
 * \brief class to be used to add Weaving to DirectX 9 based SR applications
 *
 * \ingroup API
 */
namespace SR
{
    class DIMENCOSR_API DX9WeaverBase {
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
        DX9WeaverBase();
        ~DX9WeaverBase();

        /*!
         * \brief Returns the buffer that will be used to create a weaved image. This buffer expects a side-by-side image.
         * When a frame buffer is provided by calling setInputFrameBuffer(), the internally created buffer will be released.
         *
         * The buffer identifier can be bound using `SetRenderTarget` to start rendering to it.
         * Rendering to the buffer is identical to normal rendering and often starts with calling `Clear`.
         * \returns IDirect3DSurface9 identifying a buffer generated with `CreateTexture`
         */
        IDirect3DSurface9* getFrameBuffer();

        /*!
         * \brief Sets the buffer that will be used to create a weaved image. This will release the internally created frame buffer.
         * \param frameBuffer Frame buffer to set
         */
        void setInputFrameBuffer(IDirect3DTexture9* frameBuffer);

        /*!
         * \brief Sets the window handle of the application window.
         * \param window Handle of the application window
         */
        void setWindowHandle(HWND handle);

        /*!
         * \brief Used to determine if software weaving is possible for certain size and visibility to the currently bound framebuffer
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * \throw std::runtime_error if the window handle (HWND) becomes invalid during the execution of canWeave
         * \returns bool indicating whether weaving can be done. Returns true when software weaving can be performed by DX9Weaver. When false is returned, you can output side-by-side manually or let DX9Weaver handle this.
         */
        bool canWeave(unsigned int width, unsigned int height);

        /*!
         * \brief Used to determine if software weaving is possible for certain size and visibility to the currently bound framebuffer
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * \param xOffset of the image to be rendered to the bound framebuffer
         * \param yOffset of the image to be rendered to the bound framebuffer
         * \throw std::runtime_error if the window handle (HWND) becomes invalid during the execution of canWeave
         * \returns bool indicating whether weaving can be done. Returns true when software weaving can be performed by DX9Weaver. When false is returned, you can output side-by-side manually or let DX9Weaver handle this.
         */
        bool canWeave(unsigned int width, unsigned int height, unsigned int xOffset, unsigned int yOffset);

        /*!
         * \brief Can be called to render a weaved image of a certain size to the currently bound framebuffer
         * A framebuffer must be set before calling this function.
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         */
        void weave(unsigned int width, unsigned int height);

        /*!
         * \brief Can be called to render a weaved image of a certain size to the currently bound framebuffer
         * A framebuffer must be set before calling this function.
         * \param width of the image to be rendered to the bound framebuffer
         * \param height of the image to be rendered to the bound framebuffer
         * \param xOffset of the image to be rendered to the bound framebuffer
         * \param yOffset of the image to be rendered to the bound framebuffer
         */
        void weave(unsigned int width, unsigned int height, unsigned int xOffset, unsigned int yOffset);

        /*!
         * \brief Free all resources that were created in the default memory pool. This method should be called before IDirect3DDevice9::Reset().
         */
        void invalidateDeviceObjects();

        /*!
         * \brief Allocate all resources that are created in the default memory pool. This method should be called after IDirect3DDevice9::Reset().
         */
        void restoreDeviceObjects();
    };

    class DIMENCOSR_API DX9Weaver : public DX9WeaverBase {
    public:
        /*!
         * \brief Constructs a class to be used for weaving an input image of a certain size
         * \param context to connect to
         * \param device interface used to create resources
         * \param width of the side-by-side image to be weaved together
         * \param height of the side-by-side image to be weaved together
         * \param window Handle of the application window
         */
        DX9Weaver(SR::SRContext& context, IDirect3DDevice9* device, unsigned int width, unsigned int height, HWND window);

        /*!
         * \brief Handles proper destruction of all weaver related classes and buffers
         */
        ~DX9Weaver();
    };

    class DIMENCOSR_API PredictingDX9Weaver : public DX9WeaverBase {
    public:
        /*!
         * \brief Constructs a class to be used for weaving an input image of a certain size
         * \param context to connect to
         * \param device interface used to create resources
         * \param width of the side-by-side image to be weaved together
         * \param height of the side-by-side image to be weaved together
         * \param window Handle of the application window
         */
        PredictingDX9Weaver(SR::SRContext& context, IDirect3DDevice9* device, unsigned int width, unsigned int height, HWND window);

        /*!
         * \brief Handles proper destruction of all weaver related classes and buffers
         */
        ~PredictingDX9Weaver();

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
