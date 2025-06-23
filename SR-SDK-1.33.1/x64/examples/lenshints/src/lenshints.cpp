/*!
 * Copyright (C) 2025 Leia, Inc.
 */

#include <iostream>
#include "sr/sense/display/switchablehint.h"

// Returns true if the application should terminate
bool handleInput(char input, SR::SwitchableLensHint* lensHint) {
    if (input == 'd') {
        // Allow the lens to be disabled if the system decides this is desirable
        lensHint->disable();

        // Verify if the lens is disabled after a second.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Lens state: " << lensHint->isEnabled() << std::endl;

        // Keep running
        return true;
    }
    else if (input == 'e') {
        // Request the lens to be enabled if the system decides this is desirable
        lensHint->enable();

        // Verify if lens is enabled after a second.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Lens state: " << lensHint->isEnabled() << std::endl;

        // Keep running
        return true;
    }
    else if (input == 'x') {
        // Exit the application
        return false;
    }
    else {
        std::cout << "Unknown input character!" << std::endl;

        // Print current state of the lens
        std::cout << "Lens state: " << lensHint->isEnabled() << std::endl;

        // Keep running
        return true;
    }
}

int main() {
    // Initialize SR classes
    SR::SRContext context;
    SR::SwitchableLensHint* lensHint = SR::SwitchableLensHint::create(context);
    context.initialize();

    while (true) {
        // Instruct the user
        std::cout << "Press 'd' to disable the lens, 'e' to enable the lens or 'x' to exit the application!" << std::endl;

        // Wait for input character
        char inchar;
        std::cin >> inchar;

        // Handle input
        if (!handleInput(inchar, lensHint)) {
            // Break from main loop
            break;
        };
    }

    // End the application
    std::cout << "Stopping" << std::endl;
}
