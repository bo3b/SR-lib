/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

#ifdef WIN32
#   ifdef COMPILING_DLL_SimulatedRealityCore
#     define DIMENCOSR_API __declspec(dllexport)
#   else
#     define DIMENCOSR_API __declspec(dllimport)
#   endif
#else
#   define DIMENCOSR_API
#endif

namespace SR {

/**
 * \brief Interface defining how to send messages between SR applications
 *
 * \ingroup Core_Network API
 */
class DIMENCOSR_API NetworkInterface {
public:
    /**
     * \brief Send data over NetworkInterface
     *
     * \param destination represents the remote instance to receive message
     * \param payload local address of data to be sent over the NetworkInterface
     * \param payloadSize size in memory of data to be sent over the NetworkInterface
     */
    virtual void send(uint64_t destination, void* payload, uint64_t payloadSize) = 0;

    /**
     * \brief Returns whether the connection is active
     *
     * \return bool indicating whether network messages can be received by this interface
     *
     * Can be used to determine when related instances can be cleaned up
     */
    virtual bool isActive() = 0;
};

}

#undef DIMENCOSR_API
