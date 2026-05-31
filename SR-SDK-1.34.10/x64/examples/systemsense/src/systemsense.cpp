/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>

#include "sr/sense/system/systemsense.h"
#include "sr/sense/system/systemevent.h"
#include "sr/sense/core/inputstream.h"
#include "sr/utility/exception.h"

using namespace std::chrono_literals;

// User implementation of the SystemEventListener interface
class SystemEventMonitor : public SR::SystemEventListener {
public:
    // Ensures SystemEventStream is cleaned up when Listener object is out of scope
    SR::InputStream<SR::SystemEventStream> stream;

    // Flag to indicate that context is no longer valid
    bool contextValid = true;

    // The accept function can process the system event data as soon as it becomes available
    virtual void accept(const SR::SystemEvent& frame) override {
        switch (frame.eventType) {
        case SR_eventType::Info:
            std::cout << "Info" << std::endl;
            break;
        case SR_eventType::ContextInvalid:
            std::cout
                << frame.time << " "
                << frame.message << "\n";
            contextValid = false;
            break;
        case SR_eventType::SRUnavailable:
            std::cout << "Only display 2D content" << std::endl;
            break;
        case SR_eventType::SRRestored:
            std::cout << "Display SR content" << std::endl;
            break;
        case SR_eventType::USBNotConnected:
            std::cout << "USBNotConnected" << std::endl;
            break;
        case SR_eventType::USBNotConnectedResolved:
            std::cout << "USBNotConnectedResolved" << std::endl;
            break;
        case SR_eventType::DisplayNotConnected:
            std::cout << "DisplayNotConnected" << std::endl;
            break;
        case SR_eventType::DisplayNotConnectedResolved:
            std::cout << "DisplayNotConnectedResolved" << std::endl;
            break;
        case SR_eventType::Duplicated:
            std::cout << "Duplicated" << std::endl;
            break;
        case SR_eventType::DuplicatedResolved:
            std::cout << "DuplicatedResolved" << std::endl;
            break;
        case SR_eventType::NonNativeResolution:
            std::cout << "NonNativeResolution" << std::endl;
            break;
        case SR_eventType::NonNativeResolutionResolved:
            std::cout << "NonNativeResolutionResolved" << std::endl;
            break;
        case SR_eventType::DeviceConnectedAndReady:
            std::cout << "DeviceConnectedAndReady" << std::endl;
            break;
        case SR_eventType::DeviceDisconnected:
            std::cout << "DeviceDisconnected" << std::endl;
            break;
        case SR_eventType::LensOn:
            std::cout << "LensOn" << std::endl;
            break;
        case SR_eventType::LensOff:
            std::cout << "LensOff" << std::endl;
            break;
        case SR_eventType::UserLost:
            std::cout << "UserLost" << std::endl;
            break;
        case SR_eventType::UserFound:
            std::cout << "UserFound" << std::endl;
            break;
        default:
            std::cout << "Unknown event type" << std::endl;
            break;
        }
    }

};

int main() {
    std::mutex contextMutex;

    // Create SRContext
    SR::SRContext* context = nullptr;

    while (true) {
        context = nullptr;

        // Initialize SRContext
        while (context == nullptr) {
            try {
                context = new SR::SRContext();
            }
            catch (SR::ServerNotAvailableException e) {
                std::cout << "Server not available, trying again in 1 second" << std::endl;
                std::this_thread::sleep_for(1000ms);
            }
        }

        SR::SystemSense* systemSense = SR::SystemSense::create(*context);

        // Construct listener
        SystemEventMonitor listener;
        listener.stream.set(systemSense->openSystemEventStream(&listener));

        // Start listening
        context->initialize();

        // Wait until ContextInvalid is received
        while (listener.contextValid) {
            std::this_thread::sleep_for(10ms);
        }

        // Delete context
        delete context;

        // Reinitialize
    }


}
