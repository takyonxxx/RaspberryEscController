#include "mainwindow.h"

MotorPWMWidget::MotorPWMWidget(const QString &labelText, QWidget *parent)
    : QWidget(parent)
    , m_currentPWM(PWM_NEUTRAL)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Motor label
    m_label = new QLabel(labelText, this);
    m_label->setStyleSheet("color: white; background-color: #FF4633; padding: 10px; border-radius: 5px;");
    QFont labelFont;
    labelFont.setPointSize(16);
    labelFont.setBold(true);
    m_label->setFont(labelFont);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setMinimumHeight(40);

    // PWM value display
    m_valueLabel = new QLabel(QString::number(PWM_NEUTRAL), this);
    m_valueLabel->setStyleSheet("color: white; background-color: #336699; padding: 8px; border-radius: 5px;");
    QFont valueFont;
    valueFont.setPointSize(14);
    valueFont.setBold(true);
    m_valueLabel->setFont(valueFont);
    m_valueLabel->setAlignment(Qt::AlignCenter);
    m_valueLabel->setMinimumHeight(35);

    // PWM slider (vertical)
    m_pwmSlider = new QSlider(Qt::Vertical, this);
    m_pwmSlider->setRange(PWM_MIN, PWM_MAX);
    m_pwmSlider->setValue(PWM_NEUTRAL);
    m_pwmSlider->setTickPosition(QSlider::TicksLeft);
    m_pwmSlider->setTickInterval(100);
    m_pwmSlider->setMinimumHeight(200);
    // Reduce slider tracking sensitivity for better performance
    m_pwmSlider->setSingleStep(5);  // 5μs steps
    m_pwmSlider->setPageStep(50);   // 50μs page steps
    m_pwmSlider->setStyleSheet(
        "QSlider::groove:vertical {"
        "    background: #E0E0E0;"
        "    width: 8px;"
        "    border-radius: 4px;"
        "}"
        "QSlider::handle:vertical {"
        "    background: #336699;"
        "    border: 2px solid #336699;"
        "    height: 20px;"
        "    border-radius: 10px;"
        "    margin: 0 -6px;"
        "}"
        "QSlider::handle:vertical:hover {"
        "    background: #4A7BA7;"
        "}"
        "QSlider::sub-page:vertical {"
        "    background: #336699;"
        "    border-radius: 4px;"
        "}"
        );

    // Neutral button
    m_neutralButton = new QPushButton("Neutral", this);
    m_neutralButton->setStyleSheet(
        "QPushButton {"
        "    color: white;"
        "    background-color: #FF6B35;"
        "    padding: 8px;"
        "    border-radius: 5px;"
        "    font-size: 12px;"
        "    font-weight: bold;"
        "    min-height: 25px;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #CC5529;"
        "}"
        );

    // Layout
    mainLayout->addWidget(m_label);
    mainLayout->addWidget(m_valueLabel);
    mainLayout->addWidget(m_pwmSlider, 1);
    mainLayout->addWidget(m_neutralButton);

    // Connections
    connect(m_pwmSlider, &QSlider::valueChanged, this, &MotorPWMWidget::onSliderValueChanged);
    connect(m_neutralButton, &QPushButton::clicked, [this]() {
        setPWMValue(PWM_NEUTRAL);
    });
}

void MotorPWMWidget::setPWMValue(int pwmValue)
{
    pwmValue = qBound(PWM_MIN, pwmValue, PWM_MAX);

    if (m_currentPWM != pwmValue) {
        m_currentPWM = pwmValue;
        m_pwmSlider->setValue(pwmValue);
        m_valueLabel->setText(QString::number(pwmValue));

        // Update value label color based on PWM value
        if (pwmValue < PWM_NEUTRAL) {
            m_valueLabel->setStyleSheet("color: white; background-color: #C0392B; padding: 8px; border-radius: 5px;"); // Red for reverse
        } else if (pwmValue > PWM_NEUTRAL) {
            m_valueLabel->setStyleSheet("color: white; background-color: #27AE60; padding: 8px; border-radius: 5px;"); // Green for forward
        } else {
            m_valueLabel->setStyleSheet("color: white; background-color: #336699; padding: 8px; border-radius: 5px;"); // Blue for neutral
        }

        emit pwmValueChanged(pwmValue);
    }
}

int MotorPWMWidget::getPWMValue() const
{
    return m_currentPWM;
}

void MotorPWMWidget::onSliderValueChanged(int value)
{
    setPWMValue(value);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scaleFactor(1.0f)
    , m_bleConnection(nullptr)
    , m_isArmed(false)
    , m_pendingMotorNumber(0)
    , m_hasPendingPWMCommand(false)
{
    setWindowTitle(tr("4-Motor PWM Remote Control"));

#ifdef Q_OS_ANDROID
    m_scaleFactor = 1.25f;
#endif

    // Setup PWM command throttling timer (send at most every 50ms)
    m_pwmSendTimer = new QTimer(this);
    m_pwmSendTimer->setSingleShot(true);
    m_pwmSendTimer->setInterval(20); // 20ms = 50Hz update rate
    connect(m_pwmSendTimer, &QTimer::timeout, this, &MainWindow::sendPendingPWMCommands);

    createUI();
    setupBluetoothConnection();
    setupConnections();

    statusChanged("No device connected.");
}

MainWindow::~MainWindow()
{
    if (m_bleConnection) {
        m_bleConnection->disconnectFromDevice();
        delete m_bleConnection;
    }
}

void MainWindow::createUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);

    // Status label
    m_textStatus = new QLabel(this);
    m_textStatus->setStyleSheet("color: #cccccc; background-color: #003333; border-radius: 5px; padding: 10px;");
    QFont statusFont;
    statusFont.setPointSize(static_cast<int>(12 * m_scaleFactor));
    m_textStatus->setFont(statusFont);
    m_textStatus->setMaximumHeight(60);

    // Connection and control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    m_connectButton = new QPushButton("Connect", this);
    m_armedButton = new QPushButton("Arm", this);
    m_allNeutralButton = new QPushButton("All Neutral", this);
    m_exitButton = new QPushButton("Exit", this);

    QString buttonStyle = "QPushButton { color: white; background-color: #336699; padding: 10px; "
                          "border-radius: 8px; font-size: 14px; font-weight: bold; min-height: 25px; } "
                          "QPushButton:pressed { background-color: #1E3F5A; }";

    m_connectButton->setStyleSheet(buttonStyle);
    m_armedButton->setStyleSheet(buttonStyle);
    m_allNeutralButton->setStyleSheet(buttonStyle);
    m_exitButton->setStyleSheet(buttonStyle);

    buttonLayout->addWidget(m_connectButton);
    buttonLayout->addWidget(m_armedButton);
    buttonLayout->addWidget(m_allNeutralButton);
    buttonLayout->addWidget(m_exitButton);

    // Motor PWM controls group
    QGroupBox *motorGroup = new QGroupBox("Motor PWM Controls", this);
    motorGroup->setStyleSheet("QGroupBox { color: black; font-size: 14px; font-weight: bold; "
                              "border: 2px solid #336699; border-radius: 10px; margin-top: 15px; padding: 10px; } "
                              "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 10px; }");

    QVBoxLayout *motorGroupLayout = new QVBoxLayout(motorGroup);
    motorGroupLayout->setSpacing(5);

    // PWM info label
    QLabel *pwmInfoLabel = new QLabel("1000=Reverse | 1500=Neutral | 2000=Forward", this);
    pwmInfoLabel->setStyleSheet("color: #666666; font-size: 14px; font-weight: normal; padding: 5px;");
    pwmInfoLabel->setAlignment(Qt::AlignCenter);
    pwmInfoLabel->setWordWrap(true);

    // Motor controls layout
    QHBoxLayout *motorLayout = new QHBoxLayout();
    motorLayout->setSpacing(15);

    m_motor1PWM = new MotorPWMWidget("Motor 1", this);
    m_motor2PWM = new MotorPWMWidget("Motor 2", this);
    m_motor3PWM = new MotorPWMWidget("Motor 3", this);
    m_motor4PWM = new MotorPWMWidget("Motor 4", this);

    m_motor1PWM->setMinimumWidth(100);
    m_motor2PWM->setMinimumWidth(100);
    m_motor3PWM->setMinimumWidth(100);
    m_motor4PWM->setMinimumWidth(100);

    motorLayout->addWidget(m_motor1PWM);
    motorLayout->addWidget(m_motor2PWM);
    motorLayout->addWidget(m_motor3PWM);
    motorLayout->addWidget(m_motor4PWM);

    // Add info label and motor controls to group
    motorGroupLayout->addWidget(pwmInfoLabel);
    motorGroupLayout->addLayout(motorLayout);

    // Add to main layout
    m_mainLayout->addWidget(m_textStatus);
    m_mainLayout->addLayout(buttonLayout);
    m_mainLayout->addWidget(motorGroup, 1);
}

void MainWindow::setupConnections()
{
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);
    connect(m_armedButton, &QPushButton::clicked, this, &MainWindow::onArmedClicked);
    connect(m_allNeutralButton, &QPushButton::clicked, this, &MainWindow::onAllNeutralClicked);

    connect(m_motor1PWM, &MotorPWMWidget::pwmValueChanged, this, &MainWindow::onMotor1PWMChanged);
    connect(m_motor2PWM, &MotorPWMWidget::pwmValueChanged, this, &MainWindow::onMotor2PWMChanged);
    connect(m_motor3PWM, &MotorPWMWidget::pwmValueChanged, this, &MainWindow::onMotor3PWMChanged);
    connect(m_motor4PWM, &MotorPWMWidget::pwmValueChanged, this, &MainWindow::onMotor4PWMChanged);
}

void MainWindow::setupBluetoothConnection()
{
    m_bleConnection = new BluetoothClient();

    connect(m_bleConnection, &BluetoothClient::statusChanged, this, &MainWindow::statusChanged);
    connect(m_bleConnection, &BluetoothClient::changedState, this, &MainWindow::changedState);
}

void MainWindow::statusChanged(const QString &status)
{
    m_textStatus->setText(status);
    update();
}

void MainWindow::changedState(BluetoothClient::bluetoothleState state)
{
    switch(state) {
    case BluetoothClient::Scanning:
        statusChanged("Searching for low energy devices...");
        break;

    case BluetoothClient::ScanFinished:
        statusChanged("Scan finished...");
        break;

    case BluetoothClient::Connecting:
        statusChanged("Connecting to device...");
        break;

    case BluetoothClient::Connected:
        m_connectButton->setText("Disconnect");
        statusChanged("Connected successfully.");
        connect(m_bleConnection, &BluetoothClient::newData, this, &MainWindow::DataHandler);
        break;

    case BluetoothClient::DisConnected:
        statusChanged("Device disconnected.");
        m_connectButton->setEnabled(true);
        m_connectButton->setText("Connect");
        m_isArmed = false;
        m_armedButton->setText("Arm");

        // Reset all motors to neutral
        m_motor1PWM->setPWMValue(1500);
        m_motor2PWM->setPWMValue(1500);
        m_motor3PWM->setPWMValue(1500);
        m_motor4PWM->setPWMValue(1500);
        break;

    case BluetoothClient::ServiceFound:
        break;

    case BluetoothClient::AcquireData:
        // Request current armed state
        // requestData(mArmed); // Uncomment if you have this functionality
        break;

    case BluetoothClient::Error:
        statusChanged("ERROR: Bluetooth operation failed.");
        break;

    default:
        break;
    }
}

void MainWindow::DataHandler(QByteArray data)
{
    uint8_t parsedCommand;
    uint8_t rw;
    QByteArray parsedValue;
    parseMessage(&data, parsedCommand, parsedValue, rw);

    if(rw == mWrite) {
        switch(parsedCommand) {
        case mArmed:
        {
            if (parsedValue.size() >= 1) {
                m_isArmed = (parsedValue.at(0) != 0);
                if(m_isArmed)
                    m_armedButton->setText("DisArm");
                else
                    m_armedButton->setText("Arm");
            }
            break;
        }
        case mData:
        {
            // Handle acknowledgment data if needed
            break;
        }
        default:
            qDebug() << "Received unknown command:" << parsedCommand;
            break;
        }
    }
    else if(rw == mRead) {
        // Handle acknowledgment messages from device
        switch(parsedCommand) {
        case mData:
        {
            // Parse PWM acknowledgment (9 bytes: 4 PWMs + channel info)
            if (parsedValue.size() >= 9) {
                // Extract PWM values (2 bytes each, little-endian)
                uint16_t pwm1 = (static_cast<uint8_t>(parsedValue.at(1)) << 8) | static_cast<uint8_t>(parsedValue.at(0));
                uint16_t pwm2 = (static_cast<uint8_t>(parsedValue.at(3)) << 8) | static_cast<uint8_t>(parsedValue.at(2));
                uint16_t pwm3 = (static_cast<uint8_t>(parsedValue.at(5)) << 8) | static_cast<uint8_t>(parsedValue.at(4));
                uint16_t pwm4 = (static_cast<uint8_t>(parsedValue.at(7)) << 8) | static_cast<uint8_t>(parsedValue.at(6));
                uint8_t channel = static_cast<uint8_t>(parsedValue.at(8));

                qDebug() << "Received acknowledgment for channel" << channel
                         << "with PWMs: Motor1=" << pwm1 << "μs, Motor2=" << pwm2
                         << "μs, Motor3=" << pwm3 << "μs, Motor4=" << pwm4 << "μs";

                // Update UI to reflect actual values (optional)
                statusChanged(QString("ACK: Ch%1 - M1:%2 M2:%3 M3:%4 M4:%5")
                                  .arg(channel).arg(pwm1).arg(pwm2).arg(pwm3).arg(pwm4));
            }
            break;
        }
        default:
            qDebug() << "Received unknown read command:" << parsedCommand;
            break;
        }
    }

    update();
}

#if defined(Q_OS_ANDROID)
void MainWindow::requestBluetoothPermissions()
{
    QBluetoothPermission bluetoothPermission;
    bluetoothPermission.setCommunicationModes(QBluetoothPermission::Access);

    switch (qApp->checkPermission(bluetoothPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(bluetoothPermission, this,
                                [this](const QPermission &permission) {
                                    if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted) {
                                        m_bleConnection->startScan();
                                    } else {
                                        statusChanged("Bluetooth permission denied.");
                                    }
                                });
        break;
    case Qt::PermissionStatus::Granted:
        m_bleConnection->startScan();
        break;
    case Qt::PermissionStatus::Denied:
        statusChanged("Bluetooth permission denied.");
        break;
    }
}
#endif

#if defined(Q_OS_IOS)
void MainWindow::requestiOSBluetoothPermissions()
{
    if (m_iOSBluetoothInitialized) {
        return;
    }

    statusChanged("Initializing Bluetooth on iOS...");

    QTimer::singleShot(1000, this, [this]() {
        statusChanged("Starting Bluetooth scan on iOS...");
        m_iOSBluetoothInitialized = true;
        m_bleConnection->startScan();
    });
}
#endif

void MainWindow::createMessage(uint8_t msgId, uint8_t rw, QByteArray payload, QByteArray *result)
{
    uint8_t buffer[MaxPayload+8] = {'\0'};
    uint8_t command = msgId;

    int len = message.create_pack(rw, command, payload, buffer);

    for (int i = 0; i < len; i++) {
        result->append(static_cast<char>(buffer[i]));
    }
}

void MainWindow::parseMessage(QByteArray *data, uint8_t &command, QByteArray &value, uint8_t &rw)
{
    MessagePack parsedMessage;

    uint8_t* dataToParse = reinterpret_cast<uint8_t*>(data->data());
    if(message.parse(dataToParse, static_cast<uint8_t>(data->length()), &parsedMessage))
    {
        command = parsedMessage.command;
        rw = parsedMessage.rw;
        for(int i = 0; i < parsedMessage.len; i++)
        {
            value.append(static_cast<char>(parsedMessage.data[i]));
        }
    }
}

void MainWindow::sendMotorPWMCommand(int motorNumber, int pwmValue)
{
    if (!m_isArmed) {
        statusChanged("System not armed - PWM commands ignored");
        return;
    }

    // Create payload with all 4 motor PWM values (8 bytes total)
    QByteArray payload;

    // Get current PWM values for all motors
    int pwm1 = m_motor1PWM->getPWMValue();
    int pwm2 = m_motor2PWM->getPWMValue();
    int pwm3 = m_motor3PWM->getPWMValue();
    int pwm4 = m_motor4PWM->getPWMValue();

    // Update the specific motor that changed
    switch(motorNumber) {
    case 1: pwm1 = pwmValue; break;
    case 2: pwm2 = pwmValue; break;
    case 3: pwm3 = pwmValue; break;
    case 4: pwm4 = pwmValue; break;
    }

    // Pack all 4 PWM values (2 bytes each, little-endian)
    payload.append(static_cast<char>(pwm1 & 0xFF));        // PWM1 low byte
    payload.append(static_cast<char>((pwm1 >> 8) & 0xFF)); // PWM1 high byte
    payload.append(static_cast<char>(pwm2 & 0xFF));        // PWM2 low byte
    payload.append(static_cast<char>((pwm2 >> 8) & 0xFF)); // PWM2 high byte
    payload.append(static_cast<char>(pwm3 & 0xFF));        // PWM3 low byte
    payload.append(static_cast<char>((pwm3 >> 8) & 0xFF)); // PWM3 high byte
    payload.append(static_cast<char>(pwm4 & 0xFF));        // PWM4 low byte
    payload.append(static_cast<char>((pwm4 >> 8) & 0xFF)); // PWM4 high byte

    // Choose the appropriate servo command based on which motor changed
    uint8_t servoCommand;
    switch(motorNumber) {
    case 1: servoCommand = mSERVO1; break;
    case 2: servoCommand = mSERVO2; break;
    case 3: servoCommand = mSERVO3; break;
    case 4: servoCommand = mSERVO4; break;
    default: servoCommand = mSERVO1; break;
    }

    QByteArray message;
    createMessage(servoCommand, mWrite, payload, &message);

    m_bleConnection->writeData(message);

    qDebug() << "Sent PWM values via SERVO" << motorNumber << "- Motor1:" << pwm1 << "Motor2:" << pwm2 << "Motor3:" << pwm3 << "Motor4:" << pwm4;
}

void MainWindow::onConnectClicked()
{
    if(m_connectButton->text() == QString("Connect"))
    {
#if defined(Q_OS_ANDROID)
        requestBluetoothPermissions();
#elif defined(Q_OS_IOS)
        requestiOSBluetoothPermissions();
#else
        statusChanged("Starting Bluetooth scan...");
        m_bleConnection->startScan();
#endif
    }
    else
    {
        m_textStatus->clear();
        m_bleConnection->disconnectFromDevice();
    }
}

void MainWindow::onExitClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Exit Application",
        "Are you sure you want to exit?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply == QMessageBox::Yes) {
        QApplication::quit();
    }
}

void MainWindow::onArmedClicked()
{
    if(!m_isArmed)
    {
        QByteArray payload;
        payload.append(static_cast<char>(1));
        QByteArray message;
        createMessage(mArmed, mWrite, payload, &message);
        m_bleConnection->writeData(message);

        m_isArmed = true;
        m_armedButton->setText("DisArm");
        statusChanged("System ARMED - PWM commands enabled");
    }
    else
    {
        QByteArray payload;
        payload.append(static_cast<char>(0));
        QByteArray message;
        createMessage(mDisArmed, mWrite, payload, &message);
        m_bleConnection->writeData(message);

        m_isArmed = false;
        m_armedButton->setText("Arm");
        statusChanged("System DISARMED - PWM commands disabled");

        // Set all motors to neutral when disarming
        onAllNeutralClicked();
    }
}

void MainWindow::onAllNeutralClicked()
{
    m_motor1PWM->setPWMValue(1500);
    m_motor2PWM->setPWMValue(1500);
    m_motor3PWM->setPWMValue(1500);
    m_motor4PWM->setPWMValue(1500);

    // Send neutral command immediately
    if (m_isArmed) {
        sendMotorPWMCommand(1, 1500); // This will send all 4 motors to neutral
    }
}

void MainWindow::onMotor1PWMChanged(int pwmValue)
{
    m_pendingMotorNumber = 1;
    m_hasPendingPWMCommand = true;

    // If timer is not running, send immediately and start timer
    if (!m_pwmSendTimer->isActive()) {
        sendMotorPWMCommand(1, pwmValue);
        m_pwmSendTimer->start();
    }
    // Otherwise, the timer will send the latest values when it expires
}

void MainWindow::onMotor2PWMChanged(int pwmValue)
{
    m_pendingMotorNumber = 2;
    m_hasPendingPWMCommand = true;

    if (!m_pwmSendTimer->isActive()) {
        sendMotorPWMCommand(2, pwmValue);
        m_pwmSendTimer->start();
    }
}

void MainWindow::onMotor3PWMChanged(int pwmValue)
{
    m_pendingMotorNumber = 3;
    m_hasPendingPWMCommand = true;

    if (!m_pwmSendTimer->isActive()) {
        sendMotorPWMCommand(3, pwmValue);
        m_pwmSendTimer->start();
    }
}

void MainWindow::onMotor4PWMChanged(int pwmValue)
{
    m_pendingMotorNumber = 4;
    m_hasPendingPWMCommand = true;

    if (!m_pwmSendTimer->isActive()) {
        sendMotorPWMCommand(4, pwmValue);
        m_pwmSendTimer->start();
    }
}

void MainWindow::sendPendingPWMCommands()
{
    if (m_hasPendingPWMCommand) {
        // Send the current PWM values for the last changed motor
        switch(m_pendingMotorNumber) {
        case 1: sendMotorPWMCommand(1, m_motor1PWM->getPWMValue()); break;
        case 2: sendMotorPWMCommand(2, m_motor2PWM->getPWMValue()); break;
        case 3: sendMotorPWMCommand(3, m_motor3PWM->getPWMValue()); break;
        case 4: sendMotorPWMCommand(4, m_motor4PWM->getPWMValue()); break;
        }
        m_hasPendingPWMCommand = false;
    }
}
