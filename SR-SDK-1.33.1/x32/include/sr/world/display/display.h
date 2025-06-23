/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

#include "sr/world/worldobject.h"
#include "sr/management/srcontext.h"

#include <fstream>

#ifdef WIN32
#   ifdef COMPILING_DLL_SimulatedRealityDisplays
#     define DIMENCOSR_API __declspec(dllexport)
#   else
#     define DIMENCOSR_API __declspec(dllimport)
#   endif
#else
#   define DIMENCOSR_API
#endif

namespace SR {

/*!
 * \brief Class of WorldObject representing the SR display in real space
 *
 * \ingroup UpstreamInterface API
 * The origin of both worlds is at the center of the display
 */
class DIMENCOSR_API Display : public WorldObject {
public:
    /**
     * \brief Creates an instance of a Display class
     *
     * \param context is the environment in which created senses are kept track of
     *
     * \return A pointer to the Display instance
     */
    static Display* create(SRContext &context);

    /**
     * \brief Destructor of the Display instance
     *
     * Explicitly defined because Impl is an incomplete type when this header is evaluated.
     * The destructor is implemented in the implementation file because the Impl class is fully defined there.
     */
    virtual ~Display() = default;

    /**
     * \brief Get the vertical resolution that is accepted by the display
     *
     * \return The vertical resolution that is accepted by the display
     */
    virtual const int getResolutionHeight() = 0;

    /**
     * \brief Get the horizontal resolution that is accepted by the display
     *
     * \return The horizontal resolution that is accepted by the display
     */
    virtual const int getResolutionWidth() = 0;

    /**
     * \brief Get the vertical native resolution of the physical display itself
     *
     * \return The vertical native resolution of the display as an integer
     */
    virtual const int getPhysicalResolutionHeight() = 0;

    /**
     * \brief Get the horizontal native resolution of the physical display itself
     *
     * \return The horizontal native resolution of the display as an integer
     */
    virtual const int getPhysicalResolutionWidth() = 0;

    /**
     * \brief Get the physical height of the display
     *
     * \return The physical height of the display as a float number in cm
     */
    virtual const float getPhysicalSizeHeight() = 0;

    /**
     * \brief Get the physical width of the display
     *
     * \return The physical width of the display as a float number in cm
     */
    virtual const float getPhysicalSizeWidth() = 0;

    /**
     * \brief Get the dot pitch (pixel size)
     *
     * \return The dot pitch (pixel size) as a float number in cm
     */
    virtual const float getDotPitch() = 0;

    /**
    * \brief Finds the SR monitor and return it's location in the display configuration
    *
    * \return A rectangle representing the position and virtual size of the display in the display configuration
    */
    virtual SR_recti getLocation() = 0;
};

}

#undef DIMENCOSR_API
