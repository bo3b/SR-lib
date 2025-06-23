/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <iostream>
#include <chrono>

#include "sr/sense/eyetracker/predictingeyetracker.h"
#include "sr/sense/eyetracker/eyetracker.h"
#include "sr/sense/eyetracker/eyepair.h"
#include "sr/sense/weavertracker/weavertracker.h"
#include "sr/sense/weavertracker/weaverposition.h"
#include "sr/sense/core/inputstream.h"

// User implementation of the EyePairListener interface
class ListenerEyePair : public SR::EyePairListener
{
public:
    // Ensures EyePairStream is cleaned up when Listener object is out of scope
    SR::InputStream<SR::EyePairStream> stream;

    // The accept function can process the eye position data as soon as it becomes available
    virtual void accept(const SR_eyePair& frame) override {
        std::cout
            << "Predicted eyepair "
            << frame.frameId << " "
            << frame.time << " "
            << frame.left.x << " " << frame.left.y << " " << frame.left.z << " "
            << frame.right.x << " " << frame.right.y << " " << frame.right.z << "\n";
    }
};

int main() {
    // Create eyetracker
    SR::SRContext context;
    bool getEyePairData = true;

    // Construct listener
    ListenerEyePair listener;

    // Construct EyeTracker to listen to
    SR::PredictingEyeTracker* eyeTracker = SR::PredictingEyeTracker::create(context);
    listener.stream.set(eyeTracker->openEyePairStream(&listener));

    // Start tracking
    context.initialize();

    //Predict 5 eyepair positions, 16ms apart, with 80us latency
    for (int i = 0; i < 100; i++) {
        using namespace std::chrono_literals;
        // Trigger eyetracker to output, this output will end up in the accept function in the listener
        eyeTracker->predict(80);
        std::this_thread::sleep_for(16ms);
    }

    std::cout << "Enter a key to end the example" << std::endl;
    char inchar;
    std::cin >> inchar; //wait for key press
}
