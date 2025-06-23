#include "esccontrolthread.h"
#include <iostream>
#include <algorithm>
#include <wiringPi.h>

// PWM constants (should match ESCControl constants)
static constexpr int PWM_NEUTRAL_US = 1500;
static constexpr int PWM_MIN_US = 1000;
static constexpr int PWM_MAX_US = 2000;

ESCControlThread::ESCControlThread()
    : m_isRunning(false)
    , m_initialized(false)
    , m_hasNewCommand(false)
{
    std::cout << "ESCControlThread created for 4 ESCs" << std::endl;
}

ESCControlThread::~ESCControlThread()
{
    stop();
    std::cout << "ESCControlThread destroyed" << std::endl;
}

bool ESCControlThread::initialize()
{
    if (m_initialized.load()) {
        std::cout << "ESCControlThread already initialized" << std::endl;
        return true;
    }

    std::cout << "Initializing ESCControlThread for 4 ESCs..." << std::endl;

    // Initialize WiringPi
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "Failed to initialize WiringPi" << std::endl;
        return false;
    }
    std::cout << "WiringPi initialized successfully" << std::endl;

    // Create all 4 ESC instances
    try {
        m_esc1 = std::make_unique<ESCControl>(PIN_ESC_1);
        m_esc2 = std::make_unique<ESCControl>(PIN_ESC_2);
        m_esc3 = std::make_unique<ESCControl>(PIN_ESC_3);
        m_esc4 = std::make_unique<ESCControl>(PIN_ESC_4);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create ESC instances: " << e.what() << std::endl;
        return false;
    }

    // Initialize all ESCs
    if (!m_esc1->initialize()) {
        std::cerr << "Failed to initialize ESC1 on pin " << PIN_ESC_1 << std::endl;
        return false;
    }

    if (!m_esc2->initialize()) {
        std::cerr << "Failed to initialize ESC2 on pin " << PIN_ESC_2 << std::endl;
        m_esc1->stop(); // Clean up ESC1 if ESC2 fails
        return false;
    }

    if (!m_esc3->initialize()) {
        std::cerr << "Failed to initialize ESC3 on pin " << PIN_ESC_3 << std::endl;
        m_esc1->stop();
        m_esc2->stop();
        return false;
    }

    if (!m_esc4->initialize()) {
        std::cerr << "Failed to initialize ESC4 on pin " << PIN_ESC_4 << std::endl;
        m_esc1->stop();
        m_esc2->stop();
        m_esc3->stop();
        return false;
    }

    // Initialize command structure with neutral positions
    m_currentCommand.esc1PulseWidth = PWM_NEUTRAL_US;
    m_currentCommand.esc2PulseWidth = PWM_NEUTRAL_US;
    m_currentCommand.esc3PulseWidth = PWM_NEUTRAL_US;
    m_currentCommand.esc4PulseWidth = PWM_NEUTRAL_US;
    m_currentCommand.emergencyStop = false;
    m_currentCommand.timestamp = std::chrono::steady_clock::now();
    m_lastCommand = m_currentCommand;

    // Start the control thread
    m_isRunning = true;
    try {
        m_controlThread = std::thread(&ESCControlThread::controlThreadFunction, this);

        // Give the thread a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        m_initialized = true;
        std::cout << "ESCControlThread initialization complete for all 4 ESCs" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to start control thread: " << e.what() << std::endl;
        m_isRunning = false;
        m_esc1->stop();
        m_esc2->stop();
        m_esc3->stop();
        m_esc4->stop();
        return false;
    }
}

void ESCControlThread::stop()
{
    if (!m_initialized.load()) {
        return;
    }

    std::cout << "Stopping ESCControlThread..." << std::endl;

    // First set all ESCs to neutral
    setAllNeutral();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the control thread
    m_isRunning = false;

    // Wake up the control thread if it's waiting
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        m_hasNewCommand = true;
    }
    m_commandCondition.notify_all();

    // Wait for the control thread to finish
    if (m_controlThread.joinable()) {
        m_controlThread.join();
    }

    // Stop all ESCs
    if (m_esc1) {
        m_esc1->stop();
    }
    if (m_esc2) {
        m_esc2->stop();
    }
    if (m_esc3) {
        m_esc3->stop();
    }
    if (m_esc4) {
        m_esc4->stop();
    }

    m_initialized = false;
    std::cout << "ESCControlThread stopped" << std::endl;
}

bool ESCControlThread::isRunning() const
{
    return m_isRunning.load() && m_initialized.load();
}

// Individual ESC control methods
void ESCControlThread::setESC1PulseWidth(int pulseWidthUs)
{
    int constrainedPulseWidth = constrainPulseWidth(pulseWidthUs);
    setCommand(constrainedPulseWidth, m_currentCommand.esc2PulseWidth,
               m_currentCommand.esc3PulseWidth, m_currentCommand.esc4PulseWidth);
    std::cout << "ESC1 pulse width set to " << constrainedPulseWidth << "μs" << std::endl;
}

void ESCControlThread::setESC2PulseWidth(int pulseWidthUs)
{
    int constrainedPulseWidth = constrainPulseWidth(pulseWidthUs);
    setCommand(m_currentCommand.esc1PulseWidth, constrainedPulseWidth,
               m_currentCommand.esc3PulseWidth, m_currentCommand.esc4PulseWidth);
    std::cout << "ESC2 pulse width set to " << constrainedPulseWidth << "μs" << std::endl;
}

void ESCControlThread::setESC3PulseWidth(int pulseWidthUs)
{
    int constrainedPulseWidth = constrainPulseWidth(pulseWidthUs);
    setCommand(m_currentCommand.esc1PulseWidth, m_currentCommand.esc2PulseWidth,
               constrainedPulseWidth, m_currentCommand.esc4PulseWidth);
    std::cout << "ESC3 pulse width set to " << constrainedPulseWidth << "μs" << std::endl;
}

void ESCControlThread::setESC4PulseWidth(int pulseWidthUs)
{
    int constrainedPulseWidth = constrainPulseWidth(pulseWidthUs);
    setCommand(m_currentCommand.esc1PulseWidth, m_currentCommand.esc2PulseWidth,
               m_currentCommand.esc3PulseWidth, constrainedPulseWidth);
    std::cout << "ESC4 pulse width set to " << constrainedPulseWidth << "μs" << std::endl;
}

void ESCControlThread::setESC1Neutral()
{
    setESC1PulseWidth(PWM_NEUTRAL_US);
    std::cout << "ESC1 set to neutral" << std::endl;
}

void ESCControlThread::setESC2Neutral()
{
    setESC2PulseWidth(PWM_NEUTRAL_US);
    std::cout << "ESC2 set to neutral" << std::endl;
}

void ESCControlThread::setESC3Neutral()
{
    setESC3PulseWidth(PWM_NEUTRAL_US);
    std::cout << "ESC3 set to neutral" << std::endl;
}

void ESCControlThread::setESC4Neutral()
{
    setESC4PulseWidth(PWM_NEUTRAL_US);
    std::cout << "ESC4 set to neutral" << std::endl;
}

// Synchronized control methods
void ESCControlThread::setAllPulseWidth(int pulseWidthUs)
{
    int constrainedPulseWidth = constrainPulseWidth(pulseWidthUs);
    setCommand(constrainedPulseWidth, constrainedPulseWidth, constrainedPulseWidth, constrainedPulseWidth);
    std::cout << "All ESCs pulse width set to " << constrainedPulseWidth << "μs" << std::endl;
}

void ESCControlThread::setAllNeutral()
{
    setAllPulseWidth(PWM_NEUTRAL_US);
    std::cout << "All ESCs set to neutral" << std::endl;
}

// Multi-ESC control
void ESCControlThread::setAllDifferentialPulseWidth(int esc1PulseWidth, int esc2PulseWidth, int esc3PulseWidth, int esc4PulseWidth)
{
    int constrainedPulseWidth1 = constrainPulseWidth(esc1PulseWidth);
    int constrainedPulseWidth2 = constrainPulseWidth(esc2PulseWidth);
    int constrainedPulseWidth3 = constrainPulseWidth(esc3PulseWidth);
    int constrainedPulseWidth4 = constrainPulseWidth(esc4PulseWidth);

    setCommand(constrainedPulseWidth1, constrainedPulseWidth2, constrainedPulseWidth3, constrainedPulseWidth4);
    std::cout << "Differential control - ESC1: " << constrainedPulseWidth1
              << "μs, ESC2: " << constrainedPulseWidth2
              << "μs, ESC3: " << constrainedPulseWidth3
              << "μs, ESC4: " << constrainedPulseWidth4 << "μs" << std::endl;
}

// Status methods
int ESCControlThread::getESC1PulseWidth() const
{
    return m_esc1 ? m_esc1->getCurrentPulseWidth() : 0;
}

int ESCControlThread::getESC2PulseWidth() const
{
    return m_esc2 ? m_esc2->getCurrentPulseWidth() : 0;
}

int ESCControlThread::getESC3PulseWidth() const
{
    return m_esc3 ? m_esc3->getCurrentPulseWidth() : 0;
}

int ESCControlThread::getESC4PulseWidth() const
{
    return m_esc4 ? m_esc4->getCurrentPulseWidth() : 0;
}

bool ESCControlThread::getESC1Status() const
{
    return m_esc1 ? m_esc1->isRunning() : false;
}

bool ESCControlThread::getESC2Status() const
{
    return m_esc2 ? m_esc2->isRunning() : false;
}

bool ESCControlThread::getESC3Status() const
{
    return m_esc3 ? m_esc3->isRunning() : false;
}

bool ESCControlThread::getESC4Status() const
{
    return m_esc4 ? m_esc4->isRunning() : false;
}

// Emergency stop
void ESCControlThread::emergencyStop()
{
    std::cout << "EMERGENCY STOP ACTIVATED!" << std::endl;
    setCommand(PWM_NEUTRAL_US, PWM_NEUTRAL_US, PWM_NEUTRAL_US, PWM_NEUTRAL_US, true);
}

// Private methods
void ESCControlThread::controlThreadFunction()
{
    std::cout << "ESC Control thread started for 4 ESCs" << std::endl;

    while (m_isRunning.load()) {
        std::unique_lock<std::mutex> lock(m_commandMutex);

        // Wait for new command or timeout
        m_commandCondition.wait_for(lock, std::chrono::milliseconds(50),
                                    [this] { return m_hasNewCommand.load() || !m_isRunning.load(); });

        if (!m_isRunning.load()) {
            break;
        }

        // Check if we have a new command to execute
        if (m_hasNewCommand.load()) {
            ESCCommand commandToExecute = m_currentCommand;
            m_hasNewCommand = false;
            lock.unlock();

            // Execute the command
            executeCommand(commandToExecute);

            // Update last command
            {
                std::lock_guard<std::mutex> updateLock(m_commandMutex);
                m_lastCommand = commandToExecute;
            }
        } else {
            lock.unlock();
        }

        // Perform safety checks
        performSafetyChecks();

        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "ESC Control thread stopped" << std::endl;
}

void ESCControlThread::executeCommand(const ESCCommand& command)
{
    if (!isCommandValid(command)) {
        std::cerr << "Invalid command received, ignoring" << std::endl;
        return;
    }

    if (command.emergencyStop) {
        std::cout << "Executing emergency stop on all ESCs" << std::endl;
        if (m_esc1) m_esc1->setPulseWidth(PWM_NEUTRAL_US);
        if (m_esc2) m_esc2->setPulseWidth(PWM_NEUTRAL_US);
        if (m_esc3) m_esc3->setPulseWidth(PWM_NEUTRAL_US);
        if (m_esc4) m_esc4->setPulseWidth(PWM_NEUTRAL_US);
        return;
    }

    // Execute the command on all ESCs
    if (m_esc1) {
        m_esc1->setPulseWidth(command.esc1PulseWidth);
    }
    if (m_esc2) {
        m_esc2->setPulseWidth(command.esc2PulseWidth);
    }
    if (m_esc3) {
        m_esc3->setPulseWidth(command.esc3PulseWidth);
    }
    if (m_esc4) {
        m_esc4->setPulseWidth(command.esc4PulseWidth);
    }
}

void ESCControlThread::setCommand(int esc1PulseWidth, int esc2PulseWidth, int esc3PulseWidth, int esc4PulseWidth, bool emergency)
{
    std::lock_guard<std::mutex> lock(m_commandMutex);

    m_currentCommand.esc1PulseWidth = esc1PulseWidth;
    m_currentCommand.esc2PulseWidth = esc2PulseWidth;
    m_currentCommand.esc3PulseWidth = esc3PulseWidth;
    m_currentCommand.esc4PulseWidth = esc4PulseWidth;
    m_currentCommand.emergencyStop = emergency;
    m_currentCommand.timestamp = std::chrono::steady_clock::now();

    m_hasNewCommand = true;
    m_commandCondition.notify_one();
}

void ESCControlThread::performSafetyChecks()
{
    // Check if all ESCs are still running
    if (m_esc1 && !m_esc1->isRunning()) {
        std::cerr << "Warning: ESC1 is not running" << std::endl;
    }
    if (m_esc2 && !m_esc2->isRunning()) {
        std::cerr << "Warning: ESC2 is not running" << std::endl;
    }
    if (m_esc3 && !m_esc3->isRunning()) {
        std::cerr << "Warning: ESC3 is not running" << std::endl;
    }
    if (m_esc4 && !m_esc4->isRunning()) {
        std::cerr << "Warning: ESC4 is not running" << std::endl;
    }

    // Check command age (prevent stale commands)
    auto now = std::chrono::steady_clock::now();
    auto commandAge = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now - m_lastCommand.timestamp).count();

    if (commandAge > 1000) { // If no command for 1 second, go to neutral
        std::cout << "No recent commands, setting all ESCs to neutral for safety" << std::endl;
        setCommand(PWM_NEUTRAL_US, PWM_NEUTRAL_US, PWM_NEUTRAL_US, PWM_NEUTRAL_US);
    }
}

bool ESCControlThread::isCommandValid(const ESCCommand& command) const
{
    // Check if all pulse widths are within valid range
    if (command.esc1PulseWidth < PWM_MIN_US || command.esc1PulseWidth > PWM_MAX_US) {
        return false;
    }
    if (command.esc2PulseWidth < PWM_MIN_US || command.esc2PulseWidth > PWM_MAX_US) {
        return false;
    }
    if (command.esc3PulseWidth < PWM_MIN_US || command.esc3PulseWidth > PWM_MAX_US) {
        return false;
    }
    if (command.esc4PulseWidth < PWM_MIN_US || command.esc4PulseWidth > PWM_MAX_US) {
        return false;
    }

    return true;
}

int ESCControlThread::constrainPulseWidth(int pulseWidth) const
{
    return std::max(PWM_MIN_US, std::min(PWM_MAX_US, pulseWidth));
}
