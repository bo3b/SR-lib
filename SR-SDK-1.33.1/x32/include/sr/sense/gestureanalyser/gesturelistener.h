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
#include "gesture.h"

namespace SR {

/**
 * \brief Interface for listening to SR_gesture updates
 *
 * \ingroup GestureAnalyser API
 */
class GestureListener {
public:
    /**
     * \brief Accept an SR_gesture frame
     *
     * \param gesture represents a new SR_gesture update
     *
     * This function is called from a GestureStream.
     * Updates will be frequent, but the next update will only be receivable once accept has returned control to the stream.
     */
    virtual void accept(const SR_gesture& gesture) = 0;
};

}
