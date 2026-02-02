#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include <RTC.h>

class SleepManager {
public:
    // Initialize the RTC and Sleep configuration
    static void begin();

    /**
     * @brief Enters Deep Sleep mode for a specific duration.
     * 
     * Actions performed:
     * 1. Shuts down the WiFi/ESP32 module completely.
     * 2. Configures the RTC alarm for the specified duration.
     * 3. Enters RA4M1 Standby mode (low power).
     * 4. Wakes up and restarts WiFi (if configured to do so by the caller later).
     * 
     * @param secondsDuration Number of seconds to sleep.
     */
    static void deepSleep(int secondsDuration);

private:
    // Callback for the RTC Interrupt
    static void alarmCallback();

    // Low-level register manipulation to enter Standby Mode
    static void enterStandbyMode(uint32_t sleepDurationS);
};

#endif // SLEEP_MANAGER_H
