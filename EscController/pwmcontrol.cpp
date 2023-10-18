#include "pwmcontrol.h"

PwmControl *PwmControl::theInstance_= nullptr;


PwmControl* PwmControl::getInstance()
{
    if (theInstance_ == nullptr)
    {
        theInstance_ = new PwmControl();
    }
    return theInstance_;
}

PwmControl::PwmControl(QObject *parent) : QThread(parent)
{
    ResetValues();

    if(!initwiringPi()) return;

}

PwmControl::~PwmControl()
{
    m_stop = true;
    softPwmWrite(PWM_ESC1, 0);
    softPwmWrite(PWM_ESC2, 0);
}

void PwmControl::ResetValues()
{
    pwmLimit = 25;
}

void PwmControl::stop()
{
    m_stop = true;
}


bool PwmControl::initwiringPi()
{
    if (wiringPiSetupPhys () < 0)
    {
        fprintf (stderr, "Unable to setup wiringPiSetupGpio: %s\n", strerror (errno)) ;
        return false;
    }
    else
    {

        softPwmCreate(PWM_ESC1,0,pwmLimit);
        softPwmWrite(PWM_ESC1, 0);
        softPwmCreate(PWM_ESC2,0,pwmLimit);
        softPwmWrite(PWM_ESC2, 0);

        qDebug("Esc wiringPi Setup ok.");
    }

    return true;
}

void PwmControl::setPwm(int port, int _pwm)
{
    pwm = _pwm;
    softPwmWrite(port, pwm);
}

void PwmControl::run()
{
//    while (!m_stop)
//    {
//        const std::lock_guard<std::mutex> lock(mutex_loop);
//        setPwm(25);
//        QThread::usleep(100);
//    }
}
