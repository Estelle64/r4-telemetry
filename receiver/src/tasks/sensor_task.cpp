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
    
    // Attente stabilisation capteur
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
                // En cas d'erreur persistante, on peut éventuellement tenter un reset soft du DHT ?
                // Pour l'instant on signale juste l'erreur.
            }
        } else {
            // Lecture réussie -> Reset compteur
            if (errorCount > 0) {
                 Serial.println("[SensorTask] Capteur DHT rétabli !");
            }
            errorCount = 0;
            setDhtStatus(true);
            
            Serial.print("[SensorTask] Local -> Temp: ");
            Serial.print(t);
            Serial.print("°C, Hum: ");
            Serial.print(h);
            Serial.println("%");

            updateLocalData(t, h);
        }

        // Lecture périodique (toutes les 5 secondes)
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}