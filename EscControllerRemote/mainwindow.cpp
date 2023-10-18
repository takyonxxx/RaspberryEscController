#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    remoteConstant = 50;

    initButtons();

    setWindowTitle(tr("Esc Remote Control"));

    ui->m_textStatus->setStyleSheet("font-size: 12pt; color: #cccccc; background-color: #003333;");
    ui->m_pBConnect->setStyleSheet("font-size: 32pt; font: bold; color: #ffffff; background-color: #336699;");
    ui->m_pBExit->setStyleSheet("font-size: 32pt; font: bold; color: #ffffff; background-color: #336699;");
    ui->labelPwmR->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #239566;");
    ui->labelPwmL->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #239566;");
    ui->labelPwmValueR->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #239566;");
    ui->labelPwmValueL->setStyleSheet("font-size: 16pt; color: #ffffff; background-color: #239566;");
    m_bleConnection = new BluetoothClient();

    connect(m_bleConnection, &BluetoothClient::statusChanged, this, &MainWindow::statusChanged);
    connect(m_bleConnection, SIGNAL(changedState(BluetoothClient::bluetoothleState)),this,SLOT(changedState(BluetoothClient::bluetoothleState)));

    connect(ui->m_pBConnect, SIGNAL(clicked()),this, SLOT(on_ConnectClicked()));   
    connect(ui->m_pBExit, SIGNAL(clicked()),this, SLOT(on_Exit()));  

    statusChanged("No Device Connected.");

}

void MainWindow::changedState(BluetoothClient::bluetoothleState state){

    switch(state){

    case BluetoothClient::Scanning:
    {
        statusChanged("Searching for low energy devices...");
        break;
    }
    case BluetoothClient::ScanFinished:
    {
        break;
    }

    case BluetoothClient::Connecting:
    {
        break;
    }
    case BluetoothClient::Connected:
    {
        ui->m_pBConnect->setText("Disconnect");
        connect(m_bleConnection, SIGNAL(newData(QByteArray)), this, SLOT(DataHandler(QByteArray)));
        break;
    }
    case BluetoothClient::DisConnected:
    {
        statusChanged("Device disconnected.");
        ui->m_pBConnect->setEnabled(true);
        ui->m_pBConnect->setText("Connect");
        break;
    }
    case BluetoothClient::ServiceFound:
    {
        break;
    }
    case BluetoothClient::AcquireData:
    {
        auto pwm = 0;
        sendCommand(mEscValue1, static_cast<uint8_t>(pwm));
        sendCommand(mEscValue2, static_cast<uint8_t>(pwm));
        break;
    }
    case BluetoothClient::Error:
    {
        ui->m_textStatus->clear();
        break;
    }
    default:
        //nothing for now
        break;
    }
}

void MainWindow::DataHandler(QByteArray data)
{
    uint8_t parsedCommand;
    uint8_t rw;
    QByteArray parsedValue;
    parseMessage(&data, parsedCommand, parsedValue, rw);
    bool ok;
    int value =  parsedValue.toHex().toInt(&ok, 16);

    if(rw == mRead)
    {

    }
    else if(rw == mWrite)
    {
        switch(parsedCommand){       
        case mData:
        {
            ui->m_textStatus->append(parsedValue.toStdString().c_str());
            break;
        }
        default:
            //nothing for now
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::initButtons(){
    /* Init Buttons */
    ui->m_pBConnect->setText("Connect");
}

void MainWindow::statusChanged(const QString &status)
{
    ui->m_textStatus->append(status);
}

void MainWindow::on_ConnectClicked()
{
    if(ui->m_pBConnect->text() == QString("Connect"))
    {
        m_bleConnection->startScan();
    }
    else
    {
        ui->m_textStatus->clear();
        m_bleConnection->disconnectFromDevice();
    }
}

void MainWindow::createMessage(uint8_t msgId, uint8_t rw, QByteArray payload, QByteArray *result)
{
    uint8_t buffer[MaxPayload+8] = {'\0'};
    uint8_t command = msgId;

    int len = message.create_pack(rw , command , payload, buffer);

    for (int i = 0; i < len; i++)
    {
        result->append(static_cast<char>(buffer[i]));
    }
}

void MainWindow::parseMessage(QByteArray *data, uint8_t &command, QByteArray &value,  uint8_t &rw)
{
    MessagePack parsedMessage;

    uint8_t* dataToParse = reinterpret_cast<uint8_t*>(data->data());
    QByteArray returnValue;
    if(message.parse(dataToParse, static_cast<uint8_t>(data->length()), &parsedMessage))
    {
        command = parsedMessage.command;
        rw = parsedMessage.rw;
        for(int i = 0; i< parsedMessage.len; i++)
        {
            value.append(static_cast<char>(parsedMessage.data[i]));
        }
    }
}

void MainWindow::requestData(uint8_t command)
{
    QByteArray payload;
    QByteArray sendData;
    createMessage(command, mRead, payload, &sendData);
    m_bleConnection->writeData(sendData);
}

void MainWindow::sendCommand(uint8_t command, uint8_t value)
{
    QByteArray payload;
    payload.resize(1);

    // Assign the value to the first element of the payload
    payload[0] = static_cast<char>(value);

    // Create the message and send it
    QByteArray sendData;
    createMessage(command, mWrite, payload, &sendData);

    m_bleConnection->writeData(sendData);
}

void MainWindow::sendString(uint8_t command, QByteArray value)
{
    QByteArray sendData;
    createMessage(command, mWrite, value, &sendData);

    m_bleConnection->writeData(sendData);
}

void MainWindow::on_Exit()
{
    exit(0);
}

void MainWindow::on_scrollPwmR_sliderMoved(int value)
{
    auto pwm = value * 25 / 100;
    sendCommand(mEscValue1, static_cast<uint8_t>(pwm));
    ui->labelPwmValueR->setText(QString::number(pwm));
}

void MainWindow::on_scrollPwmL_sliderMoved(int value)
{
    auto pwm = value * 25 / 100;
    sendCommand(mEscValue2, static_cast<uint8_t>(pwm));
    ui->labelPwmValueR->setText(QString::number(pwm));
}

