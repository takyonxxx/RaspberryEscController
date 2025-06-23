#pragma once

#include "esccontrol.h"
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

class ESCControlThread {
public:
    // ESC PWM Pins - Made public so they can be directly referenced
    static constexpr int PIN_ESC_1 = 18;  // First ESC PWM Pin (GPIO 18, Pin 12) - Hardware PWM capable
    static constexpr int PIN_ESC_2 = 12;  // Second ESC PWM Pin (GPIO 12, Pin 32) - Hardware PWM capable
    static constexpr int PIN_ESC_3 = 13;  // Third ESC PWM Pin (GPIO 13, Pin 33) - Hardware PWM capable
    static constexpr int PIN_ESC_4 = 19;  // Fourth ESC PWM Pin (GPIO 19, Pin 35) - Hardware PWM capable

    // Constructor
    ESCControlThread();

    // Destructor
    ~ESCControlThread();

    // Initialize both ESCs and start the control thread
    bool initialize();

    // Stop the control thread and ESCs
    void stop();

    // Check if the thread is running
    bool isRunning() const;

    // Individual ESC control methods (pulse width in microseconds)
    void setESC1PulseWidth(int pulseWidthUs);
    void setESC2PulseWidth(int pulseWidthUs);
    void setESC3PulseWidth(int pulseWidthUs);
    void setESC4PulseWidth(int pulseWidthUs);

    // Convenience methods for common positions
    void setESC1Neutral();
    void setESC2Neutral();
    void setESC3Neutral();
    void setESC4Neutral();

    // Synchronized control methods (all ESCs together)
    void setAllPulseWidth(int pulseWidthUs);
    void setAllNeutral();

    // Multi-ESC control (set all 4 ESCs with different values)
    void setAllDifferentialPulseWidth(int esc1PulseWidth, int esc2PulseWidth, int esc3PulseWidth, int esc4PulseWidth);

    // Get current status
    int getESC1PulseWidth() const;
    int getESC2PulseWidth() const;
    int getESC3PulseWidth() const;
    int getESC4PulseWidth() const;
    bool getESC1Status() const;
    bool getESC2Status() const;
    bool getESC3Status() const;
    bool getESC4Status() const;

    // Emergency stop
    void emergencyStop();

private:
    // ESC instances
    std::unique_ptr<ESCControl> m_esc1;
    std::unique_ptr<ESCControl> m_esc2;
    std::unique_ptr<ESCControl> m_esc3;
    std::unique_ptr<ESCControl> m_esc4;

    // Thread management
    std::thread m_controlThread;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_initialized;

    // Thread synchronization
    mutable std::mutex m_commandMutex;
    std::condition_variable m_commandCondition;
    std::atomic<bool> m_hasNewCommand;

    // Command storage (protected by mutex)
    struct ESCCommand {
        int esc1PulseWidth = 1500; // Neutral position
        int esc2PulseWidth = 1500; // Neutral position
        int esc3PulseWidth = 1500; // Neutral position
        int esc4PulseWidth = 1500; // Neutral position
        bool emergencyStop = false;
        std::chrono::steady_clock::time_point timestamp;
    };
    ESCCommand m_currentCommand;
    ESCCommand m_lastCommand;

    // Private methods
    void controlThreadFunction();
    void executeCommand(const ESCCommand& command);
    void setCommand(int esc1PulseWidth, int esc2PulseWidth, int esc3PulseWidth, int esc4PulseWidth, bool emergency = false);

    // Safety features
    void performSafetyChecks();
    bool isCommandValid(const ESCCommand& command) const;

    // Utility methods
    int constrainPulseWidth(int pulseWidth) const;
};
