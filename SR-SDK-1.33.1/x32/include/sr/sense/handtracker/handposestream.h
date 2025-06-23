/*!
 * Copyright (C) 2022  Dimenco
 *
 * This software has been provided under the Dimenco EULA. (End User License Agreement)
 * You can find the agreement at https://www.dimenco.eu/eula
 *
 * This source code is considered Protected Code under the definitions of the EULA.
 */

#pragma once

#include "handpose.h"
#include "handposelistener.h"

#ifdef WIN32
#   ifdef COMPILING_DLL_SimulatedRealityHandTrackers
#     define DIMENCOSR_API __declspec(dllexport)
#   else
#     define DIMENCOSR_API __declspec(dllimport)
#   endif
#else
#   define DIMENCOSR_API
#endif

namespace SR {

class HandTracker; //forward declaration

/**
 * \brief Stream of SR_handPose objects
 *
 * Connects a HandTracker Sense to a HandPoseListener object.
 *
 * \ingroup HandTracker API
 */
class DIMENCOSR_API HandPoseStream {
    class Impl;
    Impl* pimpl;
public:
    using Frame = SR_handPose;

    /**
     * \brief Construct a new HandPoseStream
     *
     * \param sense refers to the HandTracker providing updates
     * \param listener refers to the HandPoseListener receiving updates
     *
     * Called by the HandTracker when requested for an HandPoseListener through an openHandPoseStream call.
     */
    HandPoseStream(HandTracker* sense, HandPoseListener* listener);

    /**
     * \brief Stop listening and notify sense to stop sending new frames. (Called by listener)
     */
    void stopListening();

    /**
     * \brief Close the stream. (Called by sense)
     */
    void close();

    /**
     * \brief Send new frame to listener.
     *
     * \param handPose represents a single update
     */
    void update(Frame handPose);
};

}

#undef DIMENCOSR_API
