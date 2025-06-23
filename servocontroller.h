#ifndef SERVOCONTROLLER_H
#define SERVOCONTROLLER_H

#include <QObject>
#include <QByteArray>
#include <memory>
#include "esccontrolthread.h"
#include "gattserver.h"
#include "message.h"

class ServoController : public QObject
{
    Q_OBJECT

public:
    explicit ServoController(QObject *parent = nullptr);
    ~ServoController();

    // Initialize the servo controller system
    bool initialize();

    // Stop the servo controller system
    void stop();

    // Check if system is running
    bool isRunning() const;

    // Get system status
    bool isArmed() const { return systemArmed; }
    bool isBleConnected() const { return bleConnected; }

    // Manual control methods (for testing or emergency)
    void emergencyStop();
    void armSystem();
    void disarmSystem();

private slots:
    void onBleDataReceived(const QByteArray &data);
    void onConnectionStateChanged(bool connected);

private:
    // Handle different servo commands
    void handleServoCommand(int servoChannel, const MessagePack &message);

    // Send acknowledgment back to BLE client
    void sendAcknowledgment(int channel, int pwm1, int pwm2, int pwm3, int pwm4);

    // Validate PWM value
    int validatePwmValue(uint16_t rawPwm) const;

    // Extract PWM values from message data
    uint16_t extractPwmValue(const uint8_t* data, int offset = 0) const;

private:
    // Core components
    std::unique_ptr<ESCControlThread> escControl;
    GattServer *gattServer;
    std::unique_ptr<Message> messageParser;

    // System state
    bool systemArmed;
    bool bleConnected;
    bool initialized;

    // Constants
    static constexpr int PWM_MIN = 1000;
    static constexpr int PWM_MAX = 2000;
    static constexpr int PWM_NEUTRAL = 1500;
};

#endif // SERVOCONTROLLER_H
