#include "sleep_manager.h"
#include <WiFiS3.h> // Required to control the ESP32 module

// Flag to indicate wake-up source
volatile bool alarmWakeUp = false;

void SleepManager::begin() {
    RTC.begin();
    // Initialize with a default time base so the RTC starts ticking.
    // This value will be overwritten by syncTimeWithNTP() later.
    RTCTime startTime(1, Month::JANUARY, 2024, 0, 0, 0, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_INACTIVE);
    RTC.setTime(startTime);
}

void SleepManager::alarmCallback() {
    alarmWakeUp = true;
}

void SleepManager::deepSleep(int secondsDuration) {
    Serial.print("[SleepManager] Entering Deep Sleep for ");
    Serial.print(secondsDuration);
    Serial.println("s...");

    // 1. SHUTDOWN ESP32 (WiFi)
    WiFi.disconnect();
    WiFi.end(); 
    Serial.println("[SleepManager] ESP32: OFF");

    // 2. CONFIGURE RTC ALARM
    // Instead of resetting time, we get the current time and add the duration
    RTCTime current;
    RTC.getTime(current);
    
    // Calculate wake up time using Unix Timestamp math
    // RTCTime constructor can take a unix timestamp
    RTCTime alarmTime(current.getUnixTime() + secondsDuration);
    
    AlarmMatch match;
    match.addMatchHour();
    match.addMatchMinute();
    match.addMatchSecond();
    
    RTC.setAlarmCallback(SleepManager::alarmCallback, alarmTime, match);

    // 3. ENTER RA4M1 STANDBY
    Serial.println("[SleepManager] RA4M1: Entering Standby...");
    Serial.flush(); 

    enterStandbyMode();

    // =================================
    // SYSTEM WAKES UP HERE
    // =================================

    if (alarmWakeUp) {
        alarmWakeUp = false;
        Serial.println("[SleepManager] WAKE UP triggered by RTC.");
    }
}

void SleepManager::enterStandbyMode() {
    // --------------------------------------------------------
    // Low-level Renesas RA4M1 Register Configuration
    // --------------------------------------------------------

    // 1. Allow RTC Alarm to wake up the CPU
    R_ICU->WUPEN_b.RTCALMWUPEN = 1; 

    // 2. Unlock System Registers (required to write to SBYCR)
    R_SYSTEM->PRCR = 0xA503; 

    // 3. Configure Standby Mode
    R_SYSTEM->SBYCR_b.SSBY = 1;       // Select Software Standby
    R_SYSTEM->DPSBYCR_b.DPSBY = 0;    // Deep Standby disabled (Normal Standby is sufficient and faster to wake)

    // 4. Lock System Registers
    R_SYSTEM->PRCR = 0xA500;

    // 5. Stop SysTick
    // Prevents the system timer from waking up the CPU immediately
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // 6. Execute Wait For Interrupt (Sleep)
    __WFI();

    // 7. Restart SysTick (After Wakeup)
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    // 8. Disable Wakeup Trigger
    R_ICU->WUPEN_b.RTCALMWUPEN = 0; 
}
