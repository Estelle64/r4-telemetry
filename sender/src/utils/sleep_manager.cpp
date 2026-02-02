#include "sleep_manager.h"
#include <WiFiS3.h> // Required to control the ESP32 module
#include <Arduino_FreeRTOS.h>

// Flag to indicate wake-up source
volatile bool alarmWakeUp = false;

void SleepManager::begin() {
    // Ensure ESP32 (WiFi Module) is OFF to save power immediately (Sender mode)
    WiFi.disconnect();
    WiFi.end();

    RTC.begin();
    // Initialize with a default time base so the RTC starts ticking.
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
    // Even if not using WiFi, we ensure it's off to save power
    WiFi.disconnect();
    WiFi.end(); 
    Serial.println("[SleepManager] ESP32: OFF");

    // 2. CONFIGURE RTC ALARM
    RTCTime current;
    RTC.getTime(current);
    
    // Calculate wake up time using Unix Timestamp math
    RTCTime alarmTime(current.getUnixTime() + secondsDuration);
    
    AlarmMatch match;
    match.addMatchHour();
    match.addMatchMinute();
    match.addMatchSecond();
    
    RTC.setAlarmCallback(SleepManager::alarmCallback, alarmTime, match);

    // 3. ENTER RA4M1 STANDBY
    Serial.println("[SleepManager] RA4M1: Entering Standby...");
    Serial.flush(); 

    enterStandbyMode(secondsDuration);

    // =================================
    // SYSTEM WAKES UP HERE
    // =================================
    
    // Request a software reset to re-initialize everything cleanly
    Serial.println("[SleepManager] Waking up... Performing System Reset.");
    Serial.flush();
    NVIC_SystemReset();
}

void SleepManager::enterStandbyMode(uint32_t sleepDurationS) {
    // 1. Suspendre l'ordonnanceur pour éviter un switch de tâche pendant le réveil
    vTaskSuspendAll();

    // 2. Configuration réveil (RTC)
    R_ICU->WUPEN_b.RTCALMWUPEN = 1;
    
    // 3. Préparer le dodo
    R_SYSTEM->PRCR = 0xA503;
    R_SYSTEM->SBYCR_b.SSBY = 1;
    R_SYSTEM->PRCR = 0xA500;

    // 4. Arrêt du SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // --- SOMMEIL ---
    __DSB();
    __WFI();
    __ISB();
    // --- RÉVEIL ---

    // 5. RESTAURER LE SYSTICK IMMÉDIATEMENT
    SysTick->VAL = 0;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    // 6. COMPENSER LE TEMPS DANS FREERTOS (Désactivé)
    // vTaskStepTick n'est pas disponible dans la config Arduino FreeRTOS par défaut.
    // Les timers logiciels FreeRTOS seront "en pause" durant le sommeil.
    // vTaskStepTick(pdMS_TO_TICKS(sleepDurationS * 1000));

    // 7. RELANCER LES PÉRIPHÉRIQUES
    // Sur RA4M1, après un standby, il est parfois nécessaire de ré-autoriser 
    // l'accès aux modules si tu vois que le Serial ne répond plus.
    R_SYSTEM->PRCR = 0xA503;
    // R_MSTP->MSTPCRB &= ~(1 << 22); // Réactive l'UART si nécessaire (exemple)
    R_SYSTEM->PRCR = 0xA500;

    // 8. Nettoyage
    R_ICU->WUPEN_b.RTCALMWUPEN = 0;

    // 9. Reprendre l'ordonnanceur
    xTaskResumeAll();
}
