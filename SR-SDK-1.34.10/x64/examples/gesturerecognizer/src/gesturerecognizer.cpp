/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <iostream>

#include <sr/sense/gestureanalyser/gestureRecognizer.h>
#include "sr/sense/handtracker/handtracker.h"
#include "sr/sense/handtracker/handposestream.h"
#include "sr/sense/handtracker/handposelistener.h"
#include "sr/sense/core/inputstream.h"

#ifdef WIN32
#include <windows.h>
#endif

// The gesture recognizer functionality is only available for 64-bit Windows applications
#ifdef _WIN64

class Listener :
    public SR::HandPoseListener {
public:
    SR::InputStream<SR::HandPoseStream> poses;
    //Can use gesture classification model NN4 or NN5, see constructor documentation.
    SR::SR_gestureData gestureData;
    SR::GestureRecognizer* gestureRecognizer;

    virtual void accept(const SR_handPose& handpose) override {
        gestureData = gestureRecognizer->predict(handpose);
        switch (gestureData.gestureName) {
        case SR::SR_gestureName::FIST:
                std::cout << "Current hand gesture is FIST" << std::endl;
                break;
            case SR::SR_gestureName::FLAT:
                std::cout << "Current hand gesture is FLAT" << std::endl;
                break;
            case SR::SR_gestureName::PINCH:
                std::cout << "Current hand gesture is PINCH" << std::endl;
                break;
            case SR::SR_gestureName::PINCHGRABRELEASE:
                std::cout << "Current hand gesture is PINCHGRABRELEASE" << std::endl;
                break;
            case SR::SR_gestureName::POINT:
                std::cout << "Current hand gesture is POINT" << std::endl;
                break;
        }
    }
};

#endif

int main() {
    SR::SRContext context;

// The gesture recognizer functionality is only available for 64-bit Windows applications
#ifdef _WIN64

    SR::HandTracker* handTracker = SR::HandTracker::create(context);
    Listener listener;
    listener.poses.set(handTracker->openHandPoseStream(&listener));
    try {
        listener.gestureRecognizer = new SR::GestureRecognizer(SR::SR_gestureClassificationModel::NN4);
    }
    catch (const std::runtime_error& error) {
        std::cout << error.what() << std::endl;
    }
    catch (const SR::GestureRecognizerException& error) {
        std::cout << error.what() << std::endl;
    }

#else

    std::cout << "Gesture recognizer functionality is not supported in the 32 bits version of the SDK." << std::endl;

#endif

    context.initialize();

    std::cout << "Enter a key to end the example" << std::endl;
    char inchar;
    std::cin >> inchar; //wait for key press
}
