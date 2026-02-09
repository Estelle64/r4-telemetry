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
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    int errorCount = 0;
    const int MAX_ERRORS = 3;

    for (;;) {
        // --- SECTION CRITIQUE ---
        taskENTER_CRITICAL();
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        taskEXIT_CRITICAL();
        // ------------------------

        if (isnan(h) || isnan(t)) {
            errorCount++;
            Serial.print("[SensorTask] Echec lecture DHT (");
            Serial.print(errorCount);
            Serial.println(")");

            if (errorCount >= MAX_ERRORS) {
                setDhtStatus(false);
                // On met à jour avec NAN pour indiquer l'erreur et débloquer la tâche LoRa
                updateSensorData(NAN, NAN); 
            }
        } else {
            if (errorCount > 0) {
                 Serial.println("[SensorTask] Capteur DHT rétabli !");
            }
            errorCount = 0;
            
            Serial.print("[SensorTask] Lecture -> Temp: ");
            Serial.print(t);
            Serial.print("°C, Hum: ");
            Serial.print(h);
            Serial.println("%");

            updateSensorData(t, h);
            setDhtStatus(true);
        }

        // Lecture toutes les 2 secondes quand le système est réveillé
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
