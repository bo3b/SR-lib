/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include "sr/types.h"
#include "sr/sense/core/inputstream.h"
#include "sr/management/srcontext.h"
#include "sr/sense/handtracker/handtracker.h"
#include <iostream>

SR_point3d myPoint{ 0.0, 1.0, 2.0 };

class MyListener : public SR::HandPoseListener {
public:
    SR::InputStream<SR::HandPoseStream> inputStream;
    // Inherited via HandPoseListener
    virtual void accept(const SR_handPose& handpose) override
    {
        std::cout << handpose.palm.x << "\n";
    }
};

int main() { 
    SR::SRContext context;
    SR::HandTracker* handTracker = SR::HandTracker::create(context);
    MyListener listener;
    listener.inputStream.set(handTracker->openHandPoseStream(&listener));
    context.initialize();
    
    char c;
    std::cin >> c;
}
