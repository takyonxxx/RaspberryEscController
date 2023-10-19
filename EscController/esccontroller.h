#ifndef ESCCONTROLLER_H
#define ESCCONTROLLER_H

#include "robotcontrol.h"
#include "pwmcontrol.h"
#include "gattserver.h"

class EscController : public QObject
{
    Q_OBJECT

public:
    explicit EscController(QObject *parent = nullptr);
    ~EscController();
    void init(); 
    static EscController* getInstance();
    static EscController *theInstance_;

private:
    void createMessage(uint8_t msgId, uint8_t rw, QByteArray payload, QByteArray *result);
    bool parseMessage(QByteArray *data, uint8_t &command, QByteArray &value, uint8_t &rw);
    void sendData(uint8_t command, uint8_t value);
    void sendString(uint8_t command, QString value);

    GattServer *gattServer{};
    RobotControl *robotControl{};
    PwmControl *pwmControl{};
    Message message;

private slots:
    void onConnectionStatedChanged(bool);
    void onDataReceived(QByteArray);
    void onInfoReceived(QString);

};

#endif // ESCCONTROLLER_H
