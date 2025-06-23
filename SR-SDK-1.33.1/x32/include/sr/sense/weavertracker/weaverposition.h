/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once
#include "sr/types.h"
#include <stdint.h>


/**
 * \brief C-compatible struct containing the weaver position
 *
 * \ingroup WeaverTracker API
 */
typedef struct {
    uint64_t frameId; //!< Autoincrement frame number
    uint64_t time;    //!< Time of capture since epoch in microseconds
    SR_point3d weaverPosition; //!< Absolute weaver position in centimeters
} SR_weaverPosition;
