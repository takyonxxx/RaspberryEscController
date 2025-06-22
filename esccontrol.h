#ifndef ESCCONTROL_H
#define ESCCONTROL_H

#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <wiringPi.h>

class ESCControl
{
public:
    // Constructor - GPIO pin numarasını alır
    explicit ESCControl(int gpioPin);

    // Destructor
    ~ESCControl();

    // ESC'yi başlat (wiringPi initialization ve PWM thread başlatma)
    bool initialize();

    // ESC'yi durdur
    void stop();

    // PWM değerini ayarla (1000-2000 mikrosaniye arası)
    void setPulseWidth(int pulseWidthUs);

    // Throttle değerini ayarla (-100 ile +100 arası, 0 = neutral)
    void setThrottle(int throttlePercent);

    // Forward hareket (0-100 arası değer)
    void setForward(int power);

    // Reverse hareket (0-100 arası değer)
    void setReverse(int power);

    // Neutral pozisyona getir (1500μs)
    void setNeutral();

    // ESC'nin çalışır durumda olup olmadığını kontrol et
    bool isRunning() const;

    // Mevcut pulse width değerini al
    int getCurrentPulseWidth() const;

private:
    // PWM sabitleri
    static constexpr int PWM_FREQUENCY = 50;        // 50Hz (20ms period)
    static constexpr int PWM_PERIOD_US = 20000;     // 20ms in microseconds
    static constexpr int PWM_MIN_US = 1000;         // 1ms minimum pulse
    static constexpr int PWM_NEUTRAL_US = 1500;     // 1.5ms neutral pulse
    static constexpr int PWM_MAX_US = 2000;         // 2ms maximum pulse

    // PWM thread fonksiyonu
    void pwmGeneratorThread();

    // Pulse width'i sınırlar içinde tutar
    int constrainPulseWidth(int pulseWidthUs);

    // Throttle'ı pulse width'e çevirir
    int throttleToPulseWidth(int throttlePercent);

    // Üye değişkenler
    int m_gpioPin;                          // Kullanılacak GPIO pin
    std::atomic<int> m_pulseWidthUs;        // Mevcut pulse width (mikrosaniye)
    std::atomic<bool> m_isRunning;          // PWM thread çalışıyor mu?
    std::thread m_pwmThread;                // PWM üretici thread
    bool m_initialized;                     // Başlatılmış mı?
};

#endif // ESCCONTROL_H
