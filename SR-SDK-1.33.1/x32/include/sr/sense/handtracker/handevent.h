/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once
#include <stdint.h>
#include "handpose.h"

#ifdef __cplusplus
enum SR_handEventType : uint64_t {
    CreateHand = 0,
    DestroyHand = 1
};
#else
typedef uint64_t SR_handEventType;
#endif

/**
 * \brief C-compatible struct notifying the listener that the state of a hand has changed
 *
 * \ingroup HandTracker API
 */
typedef struct {
    uint64_t frameId; //!< Autoincrement frame number
    uint64_t time; //!< Time from epoch in microseconds
    uint64_t handId; //!< Hand identifier
    SR_handSide side; //!< Left or right hand
    SR_handEventType eventType; //!< Type of hand event
    uint64_t sender; //!< SR::HandTracker* Is valid while the event is being handled
} SR_handEvent;
