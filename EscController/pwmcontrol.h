#ifndef PWMCONTROL_H
#define PWMCONTROL_H

#include <QCoreApplication>
#include <QFile>
#include <QThread>
#include "constants.h"

#include "i2cdev.h"
#include <softPwm.h>
#include <wiringPi.h>


class PwmControl: public QThread
{
    Q_OBJECT

public:
    explicit PwmControl(QObject *parent = nullptr);
    ~PwmControl();

    static PwmControl* getInstance();

    bool initwiringPi();
    void ResetValues();
    void setPwm(int, int);
    void stop();

private:

    std::mutex mutex_loop;
    bool m_stop{false};

    int pwmLimit{0};
    int pwm;
    static PwmControl *theInstance_;

protected:
    void run() override;

};

#endif // PWMCONTROL_H
