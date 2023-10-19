#include "esccontroller.h"

EscController *EscController::theInstance_= nullptr;

EscController* EscController::getInstance()
{
    if (theInstance_ == nullptr)
    {
        theInstance_ = new EscController();
    }
    return theInstance_;
}

EscController::EscController(QObject *parent) : QObject(parent)
{   
    init();
}

EscController::~EscController()
{
    robotControl->stop();
    delete robotControl;
}

void EscController::createMessage(uint8_t msgId, uint8_t rw, QByteArray payload, QByteArray *result)
{
    uint8_t buffer[MaxPayload+8] = {'\0'};
    uint8_t command = msgId;

    int len = message.create_pack(rw , command , payload, buffer);

    for (int i = 0; i < len; i++)
    {
        result->append(buffer[i]);
    }
}

bool EscController::parseMessage(QByteArray *data, uint8_t &command, QByteArray &value,  uint8_t &rw)
{
    MessagePack parsedMessage;

    uint8_t* dataToParse = reinterpret_cast<uint8_t*>(data->data());
    QByteArray returnValue;

    if(message.parse(dataToParse, (uint8_t)data->length(), &parsedMessage))
    {
        command = parsedMessage.command;
        rw = parsedMessage.rw;

        for(int i = 0; i< parsedMessage.len; i++)
        {
            value.append(parsedMessage.data[i]);
        }

        return true;
    }

    return false;
}

void EscController::onConnectionStatedChanged(bool state)
{
    if(state)
    {
        qDebug() << "Bluetooth connection is succesfull.";
    }
    else
    {
        qDebug() << "Bluetooth connection lost.";
        pwmControl->setPwm(PWM_ESC1, 0);
        pwmControl->setPwm(PWM_ESC2, 0);
    }
}

void EscController::sendData(uint8_t command, uint8_t value)
{
    QByteArray payload;
    payload[0] = value;

    QByteArray sendData;
    createMessage(command, mWrite, payload, &sendData);
    gattServer->writeValue(sendData);
}

void EscController::sendString(uint8_t command, QString value)
{
    QByteArray sendData;
    QByteArray bytedata;
    bytedata = value.toLocal8Bit();
    createMessage(command, mWrite, bytedata, &sendData);
    gattServer->writeValue(sendData);
}

void EscController::onDataReceived(QByteArray data)
{
    uint8_t parsedCommand;
    uint8_t rw;
    QByteArray parsedValue;
    auto parsed = parseMessage(&data, parsedCommand, parsedValue, rw);

    if(!parsed)return;

    bool ok;
    int value =  parsedValue.toHex().toInt(&ok, 16);

//    qDebug() << "Read/Write" << rw << "Value : " <<  value;

    if(rw == mRead)
    {
        auto pwm_value = QString("Pwm value:") + QString::number(pwmControl->getPwm());

        switch (parsedCommand)
        {
        case mEscValue1:
        {
            sendString(mData, pwm_value);
            break;
        }
        case mEscValue2:
        {
            sendString(mData, pwm_value);
            break;
        }
        default:
            break;
        }
    }
    else if(rw == mWrite)
    {
        switch (parsedCommand)
        {
        case mEscValue1:
        {
            pwmControl->setPwm(PWM_ESC1, value);
            break;
        }
        case mEscValue2:
        {
            pwmControl->setPwm(PWM_ESC2, value);
            break;
        }        
        default:
            break;
        }
    }
}

void EscController::onInfoReceived(QString info)
{
    qDebug() << info;
}

void EscController::init()
{
    gattServer = GattServer::getInstance();
    if(gattServer)
    {
        QObject::connect(gattServer, &GattServer::connectionState, this, &EscController::onConnectionStatedChanged);
        QObject::connect(gattServer, &GattServer::dataReceived, this, &EscController::onDataReceived);
        QObject::connect(gattServer, &GattServer::sendInfo, this, &EscController::onInfoReceived);
        gattServer->startBleService();
    }

    pwmControl = PwmControl::getInstance();
    pwmControl->start();
}
