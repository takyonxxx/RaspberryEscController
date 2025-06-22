#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <iostream>
#include <wiringPi.h>
#include "esccontrol.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qInfo() << "ESC Control Test Program for Qt6";
    qInfo() << "Qt Version:" << qVersion();

    // WiringPi'yi başlat
    if (wiringPiSetupGpio() == -1) {
        qCritical() << "Failed to initialize wiringPi!";
        qCritical() << "Make sure you're running with sudo privileges";
        return -1;
    }

    qInfo() << "WiringPi initialized successfully";

    // ESC kontrolcüsünü oluştur (GPIO 18 pini için)
    ESCControl esc(18);

    // ESC'yi başlat
    if (!esc.initialize()) {
        qCritical() << "Failed to initialize ESC!";
        return -1;
    }

    qInfo() << "ESC initialized successfully";
    qInfo() << "Starting test sequence...";

    // Test 1: Neutral pozisyonda bekle
    qInfo() << "1. Neutral position (3 seconds)";
    esc.setNeutral();
    QThread::msleep(3000);

    // Test 2: Forward hareket testi
    qInfo() << "2. Forward movement test";
    for (int power = 0; power <= 50; power += 10) {
        qInfo() << "Forward power:" << power << "%";
        esc.setForward(power);
        QThread::msleep(1000);
    }

    // Neutral'a geri dön
    esc.setNeutral();
    QThread::msleep(2000);

    // Test 3: Reverse hareket testi
    qInfo() << "3. Reverse movement test";
    for (int power = 0; power <= 50; power += 10) {
        qInfo() << "Reverse power:" << power << "%";
        esc.setReverse(power);
        QThread::msleep(1000);
    }

    // Neutral'a geri dön
    esc.setNeutral();
    QThread::msleep(2000);

    // Test 4: Throttle ile kontrol
    qInfo() << "4. Throttle control test";
    int throttleValues[] = {0, 25, 50, 0, -25, -50, 0};
    for (int throttle : throttleValues) {
        qInfo() << "Throttle:" << throttle << "%";
        esc.setThrottle(throttle);
        QThread::msleep(2000);
    }

    // Test 5: PWM değeri ile direkt kontrol
    qInfo() << "5. Direct PWM control test";
    int pwmValues[] = {1500, 1600, 1700, 1500, 1400, 1300, 1500};
    for (int pwm : pwmValues) {
        qInfo() << "PWM:" << pwm << "μs";
                                    esc.setPulseWidth(pwm);
        QThread::msleep(2000);
    }

    // Test tamamlandı
    qInfo() << "=== Test completed ===";
    esc.setNeutral();
    QThread::msleep(2000);

    // ESC'yi durdur
    esc.stop();

    qInfo() << "Program finished";
    return 0;
}
