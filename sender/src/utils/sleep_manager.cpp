#include "sleep_manager.h"
#include <WiFiS3.h> // Required to control the ESP32 module

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

    enterStandbyMode();

    // =================================
    // SYSTEM WAKES UP HERE
    // =================================
    
    // Petite pause pour laisser l'horloge se stabiliser
    for(volatile int i=0; i<10000; i++); 

    if (alarmWakeUp) {
        alarmWakeUp = false;
        Serial.println("[SleepManager] WAKE UP triggered by RTC.");
    }
}

void SleepManager::enterStandbyMode() {
    // --------------------------------------------------------
    // DEBUG MODE: FAKE SLEEP
    // Le vrai Standby semble crasher le système (USB/FreeRTOS)
    // On simule le sommeil pour valider la logique LoRa en premier.
    // --------------------------------------------------------
    
    // On attend la durée prévue (calculée par l'appelant via RTC alarm, 
    // mais ici on ne peut pas facilement récupérer la durée.
    // L'appelant fait deepSleep(duration).
    
    // Hack: On fait juste un délai bloquant pour simuler le temps qui passe
    // Attention: deepSleep configure l'alarme RTC, donc l'alarme va quand même sonner !
    // On doit attendre que l'alarme sonne.
    
    Serial.println("[SleepManager] (Fake Sleep) Waiting for RTC Alarm...");
    while(!alarmWakeUp) {
        // On attend que l'interruption RTC mette le flag à true
        // On utilise delay() pour ne pas bloquer les autres tâches ? 
        // Non, le but du sleep est de TOUT arrêter.
        // Mais si on veut garder l'USB vivant, on boucle simplement.
        delay(100); 
    }
    
    // Reset du flag fait par l'appelant deepSleep, mais alarmWakeUp est mis à true par l'ISR
}
