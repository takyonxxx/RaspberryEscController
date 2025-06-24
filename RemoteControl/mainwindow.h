#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QDebug>
#include <QtWidgets>
#include <QBluetoothPermission>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QFont>
#include <QMessageBox>
#include "message.h"
#include "bluetoothclient.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QJniEnvironment>
#endif

namespace Ui {
class MainWindow;
}

class MotorPWMWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MotorPWMWidget(const QString &labelText, QWidget *parent = nullptr);

    void setPWMValue(int pwmValue);
    int getPWMValue() const;

signals:
    void pwmValueChanged(int pwmValue);

private slots:
    void onSliderValueChanged(int value);

private:
    QLabel *m_label;
    QLabel *m_valueLabel;
    QSlider *m_pwmSlider;
    QPushButton *m_neutralButton;

    int m_currentPWM;

    static constexpr int PWM_MIN = 1000;
    static constexpr int PWM_MAX = 2000;
    static constexpr int PWM_NEUTRAL = 1500;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Bluetooth and data handling
    void DataHandler(QByteArray data);
    void changedState(BluetoothClient::bluetoothleState state);
    void statusChanged(const QString &status);

    // Command sending methods
    void sendMotorPWMCommand(int motorNumber, int pwmValue);

    // Button handlers
    void onConnectClicked();
    void onExitClicked();
    void onArmedClicked();
    void onAllNeutralClicked();

    // Motor PWM change handlers
    void onMotor1PWMChanged(int pwmValue);
    void onMotor2PWMChanged(int pwmValue);
    void onMotor3PWMChanged(int pwmValue);
    void onMotor4PWMChanged(int pwmValue);

private slots:
    // Throttled sending (reduces message frequency)
    void sendPendingPWMCommands();

private:
    // Initialization methods
    void createUI();
    void setupConnections();
    void setupBluetoothConnection();

    // Message handling methods
    void createMessage(uint8_t msgId, uint8_t rw, QByteArray payload, QByteArray *result);
    void parseMessage(QByteArray *data, uint8_t &command, QByteArray &value, uint8_t &rw);

// Platform-specific methods
#if defined(Q_OS_ANDROID)
    void requestBluetoothPermissions();
#endif

#if defined(Q_OS_IOS)
    void requestiOSBluetoothPermissions();
    bool m_iOSBluetoothInitialized = false;
#endif

    // UI components
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;

    // Status area
    QLabel *m_textStatus;

    // Control buttons
    QPushButton *m_connectButton;
    QPushButton *m_armedButton;
    QPushButton *m_exitButton;
    QPushButton *m_allNeutralButton;

    // Motor PWM widgets
    MotorPWMWidget *m_motor1PWM;
    MotorPWMWidget *m_motor2PWM;
    MotorPWMWidget *m_motor3PWM;
    MotorPWMWidget *m_motor4PWM;

    // Member variables
    float m_scaleFactor;
    BluetoothClient *m_bleConnection;
    Message message;
    bool m_isArmed;

    // PWM throttling
    QTimer *m_pwmSendTimer;
    int m_pendingMotorNumber;
    bool m_hasPendingPWMCommand;
};

#endif // MAINWINDOW_H
