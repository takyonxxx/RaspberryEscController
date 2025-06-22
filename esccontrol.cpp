#include "esccontrol.h"
#include <algorithm>
#include <unistd.h>
#include <pthread.h>

ESCControl::ESCControl(int gpioPin)
    : m_gpioPin(gpioPin)
    , m_pulseWidthUs(PWM_NEUTRAL_US)
    , m_isRunning(false)
    , m_initialized(false)
{
    std::cout << "ESCControl created for GPIO pin " << m_gpioPin << std::endl;
}

ESCControl::~ESCControl()
{
    stop();
    std::cout << "ESCControl destroyed for GPIO pin " << m_gpioPin << std::endl;
}

bool ESCControl::initialize()
{
    if (m_initialized) {
        std::cout << "ESC already initialized on pin " << m_gpioPin << std::endl;
        return true;
    }

    // WiringPi'nin başlatıldığından emin olun (ana programda yapılmalı)
    // wiringPiSetupGpio(); // Bu satır main'de çağrılmalı

    // GPIO pinini output olarak ayarla
    pinMode(m_gpioPin, OUTPUT);
    digitalWrite(m_gpioPin, LOW);

    std::cout << "Initializing ESC on GPIO pin " << m_gpioPin << std::endl;
    std::cout << "PWM Settings:" << std::endl;
    std::cout << "  Frequency: " << PWM_FREQUENCY << "Hz" << std::endl;
    std::cout << "  Period: " << PWM_PERIOD_US << "μs" << std::endl;
            std::cout << "  Pulse range: " << PWM_MIN_US << "-" << PWM_MAX_US << "μs" << std::endl;
                     std::cout << "  Neutral: " << PWM_NEUTRAL_US << "μs" << std::endl;

                         // PWM thread'ini başlat
                         m_isRunning = true;
    m_pulseWidthUs = PWM_NEUTRAL_US;

    try {
        m_pwmThread = std::thread(&ESCControl::pwmGeneratorThread, this);

        // Thread'e yüksek öncelik ver
        pthread_t nativeHandle = m_pwmThread.native_handle();
        struct sched_param params;
        params.sched_priority = sched_get_priority_max(SCHED_FIFO);

        if (pthread_setschedparam(nativeHandle, SCHED_FIFO, &params) != 0) {
            std::cout << "Warning: Could not set high priority for PWM thread" << std::endl;
        }

        m_initialized = true;

        // ESC'nin neutral sinyali tanıması için kısa bir bekleme
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::cout << "ESC initialization complete on pin " << m_gpioPin << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to start PWM thread: " << e.what() << std::endl;
        m_isRunning = false;
        return false;
    }
}

void ESCControl::stop()
{
    if (!m_initialized) {
        return;
    }

    std::cout << "Stopping ESC on pin " << m_gpioPin << std::endl;

    // Önce neutral pozisyona getir
    setNeutral();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // PWM thread'ini durdur
    m_isRunning = false;

    if (m_pwmThread.joinable()) {
        m_pwmThread.join();
    }

    // Pin'i LOW pozisyonuna getir
    digitalWrite(m_gpioPin, LOW);

    m_initialized = false;
    std::cout << "ESC stopped on pin " << m_gpioPin << std::endl;
}

void ESCControl::setPulseWidth(int pulseWidthUs)
{
    int constrainedWidth = constrainPulseWidth(pulseWidthUs);
    m_pulseWidthUs = constrainedWidth;

    std::cout << "ESC pin " << m_gpioPin << ": Pulse width set to "
              << constrainedWidth << "μs" << std::endl;
}

void ESCControl::setThrottle(int throttlePercent)
{
    // Throttle'ı -100 ile +100 arasında sınırla
    throttlePercent = std::max(-100, std::min(100, throttlePercent));

    int pulseWidth = throttleToPulseWidth(throttlePercent);
    setPulseWidth(pulseWidth);

    std::cout << "ESC pin " << m_gpioPin << ": Throttle set to "
              << throttlePercent << "% (" << pulseWidth << "μs)" << std::endl;
}

void ESCControl::setForward(int power)
{
    // Power'ı 0-100 arasında sınırla
    power = std::max(0, std::min(100, power));
    setThrottle(power);

    std::cout << "ESC pin " << m_gpioPin << ": Forward power " << power << "%" << std::endl;
}

void ESCControl::setReverse(int power)
{
    // Power'ı 0-100 arasında sınırla ve negatif yap
    power = std::max(0, std::min(100, power));
    setThrottle(-power);

    std::cout << "ESC pin " << m_gpioPin << ": Reverse power " << power << "%" << std::endl;
}

void ESCControl::setNeutral()
{
    setPulseWidth(PWM_NEUTRAL_US);
    std::cout << "ESC pin " << m_gpioPin << ": Set to neutral position" << std::endl;
}

bool ESCControl::isRunning() const
{
    return m_isRunning.load() && m_initialized;
}

int ESCControl::getCurrentPulseWidth() const
{
    return m_pulseWidthUs.load();
}

void ESCControl::pwmGeneratorThread()
{
    std::cout << "PWM thread started for GPIO pin " << m_gpioPin << std::endl;

    // Sleep overhead'ini ölç
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    auto end = std::chrono::high_resolution_clock::now();

    int overhead = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() - 100;
    overhead = std::max(0, overhead);

    std::cout << "PWM thread overhead: " << overhead << "μs" << std::endl;

        while (m_isRunning.load()) {
        auto cycleStart = std::chrono::high_resolution_clock::now();

        // Mevcut pulse width'i al
        int currentPulseWidth = m_pulseWidthUs.load();

        // Pin'i HIGH yap
        digitalWrite(m_gpioPin, HIGH);

        // Pulse süresi kadar bekle
        if (currentPulseWidth > overhead) {
            std::this_thread::sleep_for(std::chrono::microseconds(currentPulseWidth - overhead));
        }

        // Kesin timing için spin-wait
        while (std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::high_resolution_clock::now() - cycleStart).count() < currentPulseWidth) {
            // Busy wait
        }

        // Pin'i LOW yap
        digitalWrite(m_gpioPin, LOW);

        // Pulse süresini hesapla
        auto pulseTime = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::high_resolution_clock::now() - cycleStart).count();

        // Periyodun geri kalanını bekle
        int remainingTime = PWM_PERIOD_US - pulseTime;
        if (remainingTime > overhead) {
            std::this_thread::sleep_for(std::chrono::microseconds(remainingTime - overhead));
        }

        // Bir sonraki cycle için kesin timing
        while (std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::high_resolution_clock::now() - cycleStart).count() < PWM_PERIOD_US) {
            // Busy wait
        }
    }

    std::cout << "PWM thread stopped for GPIO pin " << m_gpioPin << std::endl;
}

int ESCControl::constrainPulseWidth(int pulseWidthUs)
{
    return std::max(PWM_MIN_US, std::min(PWM_MAX_US, pulseWidthUs));
}

int ESCControl::throttleToPulseWidth(int throttlePercent)
{
    // Throttle'ı -100 ile +100 arasında sınırla
    throttlePercent = std::max(-100, std::min(100, throttlePercent));

    // -100 => 1000μs, 0 => 1500μs, 100 => 2000μs
    return PWM_NEUTRAL_US + (throttlePercent * (PWM_MAX_US - PWM_NEUTRAL_US) / 100);
}
