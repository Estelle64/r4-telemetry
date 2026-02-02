#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include <PubSubClient.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/time_manager.h"

// ---------------------- MQTT Setup ----------------------
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ---------------------- Task ----------------------
void wifiTask(void *pvParameters) {

    for (;;) {
        // --- 1. CONNEXION WIFI ---
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Tentative de connexion...");
            WiFi.begin(WIFI_SSID, WIFI_PASS);

            int tryCount = 0;
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
                tryCount++;

                if (tryCount > 20) {
                    WiFi.disconnect();
                    WiFi.begin(WIFI_SSID, WIFI_PASS);
                    tryCount = 0;
                    Serial.println("\n[WiFi] Retry...");
                }
            }

            Serial.println("\n[WiFi] Connecté !");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());

            // Synchronisation du temps NTP
            if (syncTimeWithNTP()) {
                setTimeSyncStatus(true);
            }

            // --- 2. CONNEXION MQTT ---
            mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
            while (!mqttClient.connected()) {
                Serial.println("[MQTT] Tentative de connexion...");
                if (mqttClient.connect("ArduinoPasserelle")) {
                    Serial.println("[MQTT] Connecté !");
                } else {
                    Serial.print("[MQTT] Échec, rc=");
                    Serial.println(mqttClient.state());
                    delay(2000);
                }
            }
        }

        // --- 3. PUBLICATION DES DONNÉES ---
        if (mqttClient.connected()) {
            SystemData data = getSystemData();

            // 1. PUBLICATION CAFETERIA (C'est nous, le Receiver/Local)
            // On envoie si le capteur local fonctionne
            if (!isnan(data.localTemperature)) {
                Serial.println("[MQTT] >>> Envoi Topic '" MQTT_TOPIC_CAFET "' (Source: LOCALE)");
                
                String payload = "{";
                payload += "\"source\":\"cafeteria\",";
                payload += "\"temperature\":" + String(data.localTemperature) + ",";
                payload += "\"humidity\":" + String(data.localHumidity) + ",";
                // Metadonnées système (utiles pour le monitoring de la passerelle)
                payload += "\"loraStatus\":" + String(data.loraModuleConnected ? "true" : "false") + ",";
                payload += "\"dhtStatus\":" + String(data.dhtModuleConnected ? "true" : "false") + ",";
                payload += "\"timeSynced\":" + String(data.timeSynced ? "true" : "false");
                payload += "}";
                
                mqttClient.publish(MQTT_TOPIC_CAFET, payload.c_str());
            }

            // 2. PUBLICATION FABLAB (C'est le Sender distant)
            // On envoie seulement si on a reçu des données LoRa valides pour le Fablab
            if (!isnan(data.fablab.temperature)) {
                Serial.println("[MQTT] >>> Envoi Topic '" MQTT_TOPIC_FABLAB "' (Source: LoRa DISTANT)");
                
                String payload = "{";
                payload += "\"source\":\"fablab\",";
                payload += "\"temperature\":" + String(data.fablab.temperature) + ",";
                payload += "\"humidity\":" + String(data.fablab.humidity) + ",";
                payload += "\"lastUpdate\":" + String(millis() - data.fablab.lastUpdate);
                payload += "}";
                
                mqttClient.publish(MQTT_TOPIC_FABLAB, payload.c_str());
            }
        }

        // Maintenir MQTT actif
        mqttClient.loop();

        // Pause 2 secondes avant prochaine publication
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
