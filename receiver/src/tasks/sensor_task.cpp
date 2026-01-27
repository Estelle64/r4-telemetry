#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "DHT.h"
#include "../config.h"
#include "../utils/data_manager.h"

// Initialisation du capteur DHT
static DHT dht(SENSOR_DHT_PIN, SENSOR_DHT_TYPE);

void sensorTask(void *pvParameters) {
    Serial.println("[SensorTask] Initialisation DHT...");
    dht.begin();
    
    delay(2000);

    for (;;) {
        // --- SECTION CRITIQUE ---
        // On empêche FreeRTOS de changer de tâche pendant la lecture sensible (quelques ms)
        taskENTER_CRITICAL();
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        taskEXIT_CRITICAL();
        // ------------------------

        if (isnan(h) || isnan(t)) {
            Serial.println("[SensorTask] Erreur de lecture DHT !");
            setDhtStatus(false);
        } else {
            // Si la lecture est OK, on considère le module connecté
            setDhtStatus(true);
            
            Serial.print("[SensorTask] Local -> Temp: ");
            Serial.print(t);
            Serial.print("°C, Hum: ");
            Serial.print(h);
            Serial.println("%");

            updateLocalData(t, h);
        }

        delay(5000);
    }
}