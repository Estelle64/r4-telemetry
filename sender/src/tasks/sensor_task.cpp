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
    
    // Attente initiale du capteur
    delay(2000);

    for (;;) {
        // --- SECTION CRITIQUE ---
        taskENTER_CRITICAL();
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        taskEXIT_CRITICAL();
        // ------------------------

        if (isnan(h) || isnan(t)) {
            Serial.println("[SensorTask] Erreur de lecture DHT !");
        } else {
            Serial.print("[SensorTask] Lecture -> Temp: ");
            Serial.print(t);
            Serial.print("°C, Hum: ");
            Serial.print(h);
            Serial.println("%");

            updateSensorData(t, h);
        }

        // Lecture toutes les 2 secondes quand le système est réveillé
        // Si le système dort, cette tâche est suspendue par le hardware
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
