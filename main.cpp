#include <QCoreApplication>
#include "servocontroller.h"
#include <iostream>
#include <signal.h>

// Global servo controller instance for signal handling
ServoController *g_servoController = nullptr;

// Signal handler for clean shutdown
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_servoController) {
        g_servoController->emergencyStop();
        g_servoController->stop();
    }
    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::cout << "Starting Balance Robot Servo Controller" << std::endl;

    // Set up signal handlers for clean shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    // Create servo controller
    ServoController servoController;
    g_servoController = &servoController;

    // Initialize the servo controller system
    if (!servoController.initialize()) {
        std::cerr << "Failed to initialize servo controller" << std::endl;
        return -1;
    }

    std::cout << "Servo controller initialized successfully" << std::endl;
    std::cout << "System ready - waiting for BLE connections..." << std::endl;
    std::cout << "Send ARM command (0x03) via BLE to enable servo control" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    // Run the Qt event loop
    int result = app.exec();

    // Clean shutdown
    std::cout << "Shutting down servo controller..." << std::endl;
    servoController.stop();

    return result;
}
