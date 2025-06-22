#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <iostream>
#include <wiringPi.h>
#include "esccontrol.h"

class ESCTestApp : public QObject
{
    Q_OBJECT

public:
    ESCTestApp(QObject *parent = nullptr) : QObject(parent), esc(18)
    {
        // ESC'yi başlat
        if (!esc.initialize()) {
            qCritical() << "Failed to initialize ESC!";
            QCoreApplication::exit(-1);
            return;
        }

        qInfo() << "ESC initialized successfully";
        qInfo() << "Starting test sequence...";

        // Test timer'ını başlat
        testTimer = new QTimer(this);
        connect(testTimer, &QTimer::timeout, this, &ESCTestApp::runTestSequence);
        testTimer->setSingleShot(true);
        testTimer->start(1000); // 1 saniye sonra teste başla
    }

    ~ESCTestApp()
    {
        esc.stop();
        qInfo() << "ESC stopped";
    }

private slots:
    void runTestSequence()
    {
        qInfo() << "=== ESC Test Sequence ===";

        // Test 1: Neutral pozisyon
        qInfo() << "Test 1: Neutral position";
        esc.setNeutral();
        QTimer::singleShot(2000, this, &ESCTestApp::testForward);
    }

    void testForward()
    {
        qInfo() << "Test 2: Forward movement";

        // Forward test sequence
        forwardPower = 0;
        forwardTimer = new QTimer(this);
        connect(forwardTimer, &QTimer::timeout, this, &ESCTestApp::incrementForward);
        forwardTimer->start(1000);
    }

    void incrementForward()
    {
        if (forwardPower <= 50) {
            qInfo() << "Forward power:" << forwardPower << "%";
            esc.setForward(forwardPower);
            forwardPower += 10;
        } else {
            forwardTimer->stop();
            forwardTimer->deleteLater();

            // Neutral'a dön ve reverse teste geç
            esc.setNeutral();
            QTimer::singleShot(2000, this, &ESCTestApp::testReverse);
        }
    }

    void testReverse()
    {
        qInfo() << "Test 3: Reverse movement";

        // Reverse test sequence
        reversePower = 0;
        reverseTimer = new QTimer(this);
        connect(reverseTimer, &QTimer::timeout, this, &ESCTestApp::incrementReverse);
        reverseTimer->start(1000);
    }

    void incrementReverse()
    {
        if (reversePower <= 50) {
            qInfo() << "Reverse power:" << reversePower << "%";
            esc.setReverse(reversePower);
            reversePower += 10;
        } else {
            reverseTimer->stop();
            reverseTimer->deleteLater();

            // Neutral'a dön ve throttle teste geç
            esc.setNeutral();
            QTimer::singleShot(2000, this, &ESCTestApp::testThrottle);
        }
    }

    void testThrottle()
    {
        qInfo() << "Test 4: Throttle control";

        throttleValues = {0, 25, 50, 0, -25, -50, 0};
        throttleIndex = 0;

        throttleTimer = new QTimer(this);
        connect(throttleTimer, &QTimer::timeout, this, &ESCTestApp::nextThrottle);
        throttleTimer->start(2000);
    }

    void nextThrottle()
    {
        if (throttleIndex < throttleValues.size()) {
            int throttle = throttleValues[throttleIndex];
            qInfo() << "Throttle:" << throttle << "%";
            esc.setThrottle(throttle);
            throttleIndex++;
        } else {
            throttleTimer->stop();
            throttleTimer->deleteLater();

            // PWM teste geç
            QTimer::singleShot(1000, this, &ESCTestApp::testPWM);
        }
    }

    void testPWM()
    {
        qInfo() << "Test 5: Direct PWM control";

        pwmValues = {1500, 1600, 1700, 1500, 1400, 1300, 1500};
        pwmIndex = 0;

        pwmTimer = new QTimer(this);
        connect(pwmTimer, &QTimer::timeout, this, &ESCTestApp::nextPWM);
        pwmTimer->start(2000);
    }

    void nextPWM()
    {
        if (pwmIndex < pwmValues.size()) {
            int pwm = pwmValues[pwmIndex];
            qInfo() << "PWM:" << pwm << "μs";
                                        esc.setPulseWidth(pwm);
            pwmIndex++;
        } else {
            pwmTimer->stop();
            pwmTimer->deleteLater();

            // Test tamamlandı
            QTimer::singleShot(1000, this, &ESCTestApp::finishTest);
        }
    }

    void finishTest()
    {
        qInfo() << "=== Test completed ===";
        esc.setNeutral();

        // 2 saniye sonra uygulamayı kapat
        QTimer::singleShot(2000, QCoreApplication::instance(), &QCoreApplication::quit);
    }

private:
    ESCControl esc;
    QTimer *testTimer = nullptr;
    QTimer *forwardTimer = nullptr;
    QTimer *reverseTimer = nullptr;
    QTimer *throttleTimer = nullptr;
    QTimer *pwmTimer = nullptr;

    int forwardPower = 0;
    int reversePower = 0;
    QVector<int> throttleValues;
    QVector<int> pwmValues;
    int throttleIndex = 0;
    int pwmIndex = 0;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Uygulama bilgileri
    app.setApplicationName("ESC Control Test");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ESC Control");

    qInfo() << "ESC Control Test Program for Qt6";
    qInfo() << "Qt Version:" << qVersion();

    // WiringPi'yi başlat
    if (wiringPiSetupGpio() == -1) {
        qCritical() << "Failed to initialize wiringPi!";
        qCritical() << "Make sure you're running with sudo privileges";
        return -1;
    }

    qInfo() << "WiringPi initialized successfully";

    // Test uygulamasını oluştur ve başlat
    ESCTestApp testApp;

    // SIGINT (Ctrl+C) yakalama
    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        qInfo() << "Application shutting down...";
    });

    return app.exec();
}

/*
Qt6 ile derleme ve çalıştırma:

1. qmake kullanarak:
   qmake esccontrol.pro
   make
   sudo ./esccontrol
*/
