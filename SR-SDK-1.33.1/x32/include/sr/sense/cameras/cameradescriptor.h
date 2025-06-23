/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

/**
 * \brief C-compatible descriptor of SR camera component
 */
struct SR_cameraDescriptor {
    union {
        uint64_t serialNumber;
        uint64_t numeralIdentifier;
    };
    uint64_t cameraTypeLength;
    const char* cameraType;
};
