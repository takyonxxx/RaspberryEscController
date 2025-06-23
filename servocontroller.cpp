#include "servocontroller.h"
#include <iostream>
#include <algorithm>
#include <wiringPi.h>

ServoController::ServoController(QObject *parent)
    : QObject(parent)
    , gattServer(nullptr)
    , systemArmed(false)
    , bleConnected(false)
    , initialized(false)
{
    std::cout << "ServoController created" << std::endl;
}

ServoController::~ServoController()
{
    stop();
    std::cout << "ServoController destroyed" << std::endl;
}

bool ServoController::initialize()
{
    if (initialized) {
        std::cout << "ServoController already initialized" << std::endl;
        return true;
    }

    std::cout << "Initializing ServoController..." << std::endl;

    // Initialize WiringPi first
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "Failed to initialize WiringPi" << std::endl;
        return false;
    }
    std::cout << "WiringPi initialized successfully" << std::endl;

    // Create ESC control thread instance
    escControl = std::make_unique<ESCControlThread>();

    // Initialize the ESC control thread (WiringPi already initialized)
    if (!escControl->initialize()) {
        std::cerr << "Failed to initialize ESC control" << std::endl;
        return false;
    }

    std::cout << "ESC Control Thread initialized successfully" << std::endl;
    std::cout << "ESC1 Pin: " << ESCControlThread::PIN_ESC_1 << std::endl;
    std::cout << "ESC2 Pin: " << ESCControlThread::PIN_ESC_2 << std::endl;

    // Initialize GATT server
    gattServer = GattServer::getInstance();
    if (!gattServer) {
        std::cerr << "Failed to get GATT server instance" << std::endl;
        return false;
    }

    // Connect BLE data reception to our servo control handler
    connect(gattServer, &GattServer::dataReceived, this, &ServoController::onBleDataReceived);
    connect(gattServer, &GattServer::connectionState, this, &ServoController::onConnectionStateChanged);

    // Start BLE service
    gattServer->startBleService();

    // Initialize message parser
    messageParser = std::make_unique<Message>();

    initialized = true;
    std::cout << "ServoController initialized - ready for BLE commands" << std::endl;
    return true;
}

void ServoController::stop()
{
    if (!initialized) {
        return;
    }

    std::cout << "Stopping ServoController..." << std::endl;

    // Disarm system and stop ESCs
    disarmSystem();

    // Stop BLE service
    if (gattServer) {
        gattServer->stopBleService();
    }

    // Stop ESC control
    if (escControl) {
        escControl->stop();
    }

    initialized = false;
    std::cout << "ServoController stopped" << std::endl;
}

bool ServoController::isRunning() const
{
    return initialized && escControl && escControl->isRunning();
}

void ServoController::emergencyStop()
{
    std::cout << "EMERGENCY STOP ACTIVATED!" << std::endl;
    systemArmed = false;
    if (escControl) {
        escControl->emergencyStop();
    }
}

void ServoController::armSystem()
{
    systemArmed = true;
    std::cout << "System ARMED" << std::endl;
}

void ServoController::disarmSystem()
{
    std::cout << "System DISARMED" << std::endl;
    systemArmed = false;
    if (escControl) {
        escControl->emergencyStop();
    }
}

void ServoController::onBleDataReceived(const QByteArray &data)
{
    // Parse the received message
    MessagePack message;
    uint8_t *rawData = (uint8_t*)data.data();

    if (!messageParser->parse(rawData, data.size(), &message)) {
        std::cerr << "Failed to parse BLE message" << std::endl;
        return;
    }

    std::cout << "Received BLE command: 0x" << std::hex << (int)message.command
              << " with " << std::dec << (int)message.len << " bytes of data" << std::endl;

    // Handle servo commands
    switch (message.command) {
    case mSERVO1: // ESC1 control
        handleServoCommand(1, message);
        break;

    case mSERVO2: // ESC2 control
        handleServoCommand(2, message);
        break;

    case mSERVO3: // Both ESCs (synchronized)
        handleServoCommand(3, message);
        break;

    case mSERVO4: // Differential control
        handleServoCommand(4, message);
        break;

    case mArmed:
        armSystem();
        break;

    case mDisArmed:
        disarmSystem();
        break;

    default:
        std::cout << "Unknown command: 0x" << std::hex << (int)message.command << std::endl;
        break;
    }
}

void ServoController::onConnectionStateChanged(bool connected)
{
    bleConnected = connected;

    if (connected) {
        std::cout << "BLE Client connected" << std::endl;
    } else {
        std::cout << "BLE Client disconnected - Emergency stop" << std::endl;
        emergencyStop();
    }
}

void ServoController::handleServoCommand(int servoChannel, const MessagePack &message)
{
    if (!systemArmed) {
        std::cout << "System not armed - ignoring servo command for channel " << servoChannel << std::endl;
        return;
    }

    // Each servo command should contain 4 PWM values (8 bytes total: 2 bytes per PWM)
    if (message.len < 8) {
        std::cerr << "Invalid PWM data length for servo command (need 8 bytes for 4 PWMs, got "
                  << (int)message.len << ")" << std::endl;
        return;
    }

    // Extract all 4 PWM values from message data
    uint16_t rawPwm1 = extractPwmValue(message.data, 0);  // Bytes 0-1: ESC1 PWM
    uint16_t rawPwm2 = extractPwmValue(message.data, 2);  // Bytes 2-3: ESC2 PWM
    uint16_t rawPwm3 = extractPwmValue(message.data, 4);  // Bytes 4-5: ESC3 PWM
    uint16_t rawPwm4 = extractPwmValue(message.data, 6);  // Bytes 6-7: ESC4 PWM

    // Validate all PWM values
    int pwmValue1 = validatePwmValue(rawPwm1);
    int pwmValue2 = validatePwmValue(rawPwm2);
    int pwmValue3 = validatePwmValue(rawPwm3);
    int pwmValue4 = validatePwmValue(rawPwm4);

    std::cout << "Servo Channel " << servoChannel << " - PWM Values: "
              << "ESC1=" << pwmValue1 << "μs, "
              << "ESC2=" << pwmValue2 << "μs, "
              << "ESC3=" << pwmValue3 << "μs, "
              << "ESC4=" << pwmValue4 << "μs" << std::endl;

        // Apply PWM values to all 4 ESCs
        switch (servoChannel) {
    case 1: // mSERVO1 - Set all 4 ESCs with received values
        escControl->setESC1PulseWidth(pwmValue1);
        escControl->setESC2PulseWidth(pwmValue2);
        escControl->setESC3PulseWidth(pwmValue3);
        escControl->setESC4PulseWidth(pwmValue4);
        std::cout << "SERVO1: All ESCs set with individual values" << std::endl;
        break;

    case 2: // mSERVO2 - Set all 4 ESCs with received values
        escControl->setESC1PulseWidth(pwmValue1);
        escControl->setESC2PulseWidth(pwmValue2);
        escControl->setESC3PulseWidth(pwmValue3);
        escControl->setESC4PulseWidth(pwmValue4);
        std::cout << "SERVO2: All ESCs set with individual values" << std::endl;
        break;

    case 3: // mSERVO3 - Set all 4 ESCs with received values
        escControl->setESC1PulseWidth(pwmValue1);
        escControl->setESC2PulseWidth(pwmValue2);
        escControl->setESC3PulseWidth(pwmValue3);
        escControl->setESC4PulseWidth(pwmValue4);
        std::cout << "SERVO3: All ESCs set with individual values" << std::endl;
        break;

    case 4: // mSERVO4 - Set all 4 ESCs with received values
        escControl->setESC1PulseWidth(pwmValue1);
        escControl->setESC2PulseWidth(pwmValue2);
        escControl->setESC3PulseWidth(pwmValue3);
        escControl->setESC4PulseWidth(pwmValue4);
        std::cout << "SERVO4: All ESCs set with individual values" << std::endl;
        break;

    default:
        std::cerr << "Invalid servo channel: " << servoChannel << std::endl;
        return;
    }

    // Send acknowledgment with all 4 PWM values
    sendAcknowledgment(servoChannel, pwmValue1, pwmValue2, pwmValue3, pwmValue4);
}

void ServoController::sendAcknowledgment(int channel, int pwm1, int pwm2, int pwm3, int pwm4)
{
    if (!gattServer) {
        return;
    }

    // Create response message with all 4 PWM values (8 bytes) + channel info
    QByteArray responseData;

    // Add PWM1 (2 bytes, little-endian)
    responseData.append((uint8_t)(pwm1 & 0xFF));
    responseData.append((uint8_t)((pwm1 >> 8) & 0xFF));

    // Add PWM2 (2 bytes, little-endian)
    responseData.append((uint8_t)(pwm2 & 0xFF));
    responseData.append((uint8_t)((pwm2 >> 8) & 0xFF));

    // Add PWM3 (2 bytes, little-endian)
    responseData.append((uint8_t)(pwm3 & 0xFF));
    responseData.append((uint8_t)((pwm3 >> 8) & 0xFF));

    // Add PWM4 (2 bytes, little-endian)
    responseData.append((uint8_t)(pwm4 & 0xFF));
    responseData.append((uint8_t)((pwm4 >> 8) & 0xFF));

    // Add channel info
    responseData.append((uint8_t)channel);

    uint8_t responseBuffer[256];
    uint8_t responseLen = messageParser->create_pack(mRead, mData, responseData, responseBuffer);

    QByteArray response((char*)responseBuffer, responseLen);
    gattServer->writeValue(response);

    std::cout << "Sent acknowledgment for channel " << channel
              << " with PWMs: " << pwm1 << ", " << pwm2 << ", " << pwm3 << ", " << pwm4 << "μs" << std::endl;
}

int ServoController::validatePwmValue(uint16_t rawPwm) const
{
    return std::max(PWM_MIN, std::min(PWM_MAX, (int)rawPwm));
}

uint16_t ServoController::extractPwmValue(const uint8_t* data, int offset) const
{
    // Extract 2-byte PWM value in little-endian format
    return (data[offset + 1] << 8) | data[offset];
}
