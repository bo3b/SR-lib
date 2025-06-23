/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

#include "sr/management/srcontext.h"

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

/**
 * \brief Pure virtual interface for controlling switchable lenses from applications
 *
 * The eventual effect is dependent on all SR applications that are currently running.
 * If there is only one SR application, this interface will provide direct control.
 *
 * \ingroup API
 */
class DIMENCOSR_API SwitchableLensHint : public Sense {
public:
    /*!
     * \brief Creates a functional SwitchableLensHints instance
     *
     * \param context is the environment in which created senses are kept track of
     * \return SwitchableLensHints* which provides access to an available switchable lens controller
     * 
     */
    static SwitchableLensHint* create(SRContext &context);

    /*!
     * \brief Virtual destructor
     */
    virtual ~SwitchableLensHint() {};

    /*!
     * \brief Expresses preference to enable the lens such that it affects the light transmitted through it
     */
    virtual void enable() = 0;

    /*!
     * \brief Expresses preference to disable the lens to minimize the effect on the light transmitted through it
     */
    virtual void disable() = 0;

    /*!
     * \brief Checks whether the Lens is currently enabled
     *
     * \returns a boolean representing whether the Lens is currently enabled
     */
    virtual bool isEnabled() = 0;

    /*!
     * \brief This function returns true if any of already connected applications enabled the lense, otherwise returns false.
     * 
     * \returns a boolean representing whether any applications have explicitly indicated that they want the lens to be on.
     * 
     * \throw std::system_error if mutex lock fails
     */
    virtual bool isEnabledByPreference() = 0;
};

}

#undef DIMENCOSR_API
