/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once
#include "eyepair.h"

namespace SR {

/**
 * \brief Interface for listening to SR_eyePair updates
 *
 * \ingroup EyeTracker API
 */
class EyePairListener {
public:
    /**
     * \brief Accept an SR_eyePair frame
     *
     * \param frame represents a new SR_eyePair update
     *
     * This function is called from an EyePairStream.
     * Updates will be frequent, but the next update will only be receivable once accept has returned control to the stream.
     */
    virtual void accept(const SR_eyePair& frame) = 0;
};

}
