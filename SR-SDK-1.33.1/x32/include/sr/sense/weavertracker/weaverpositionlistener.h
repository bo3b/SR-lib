/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once
#include "weaverposition.h"

namespace SR {

/**
 * \brief Interface for listening to SR_weaverPosition updates
 *
 * \ingroup WeaverTracker API
 */
class WeaverPositionListener {
public:
    /**
     * \brief Accept an SR_weaverPosition frame
     *
     * \param frame represents a new SR_weaverPosition update
     *
     * This function is called from an WeaverPositionStream.
     * Updates will be frequent, but the next update will only be receivable once accept has returned control to the stream.
     */
    virtual void accept(const SR_weaverPosition& frame) = 0;
};

}
