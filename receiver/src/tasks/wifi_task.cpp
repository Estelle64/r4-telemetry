#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include <ArduinoMqttClient.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/time_manager.h"

// ---------------------- MQTT Setup ----------------------
WiFiClient espClient;
MqttClient mqttClient(espClient);

// ---------------------- Task ----------------------
void wifiTask(void *pvParameters) {
    // Identification du client
    mqttClient.setId("ArduinoPasserelle");

    for (;;) {
        // --- 1. GESTION WIFI ---
        if (WiFi.status() != WL_CONNECTED) {
            setWifiStatus(false);
            setMqttStatus(false); // Si pas de WiFi, MQTT est forcément off
            
            Serial.println("[WiFi] Connexion perdue ou non établie. Tentative...");
            WiFi.begin(WIFI_SSID, WIFI_PASS);

            int tryCount = 0;
            // On attend max 10 secondes (20 * 500ms)
            while (WiFi.status() != WL_CONNECTED && tryCount < 20) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                Serial.print(".");
                tryCount++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\n[WiFi] Connecté ! IP: " + WiFi.localIP().toString());
                setWifiStatus(true);
                
                // Synchro NTP au réveil du WiFi (Première tentative)
                if (syncTimeWithNTP()) {
                    setTimeSyncStatus(true);
                }
            } else {
                 Serial.println("\n[WiFi] Échec. Prochaine tentative dans 5s...");
                 vTaskDelay(5000 / portTICK_PERIOD_MS);
                 continue; // On recommence la boucle pour re-tester le WiFi
            }
        } else {
            setWifiStatus(true); // WiFi est OK
            
            // Si l'heure n'est pas encore synchronisée, on réessaie périodiquement
            SystemData data = getSystemData();
            if (!data.timeSynced) {
                static unsigned long lastNtpTry = 0;
                if (millis() - lastNtpTry > 60000) { // Retry toutes les 60s
                    lastNtpTry = millis();
                    Serial.println("[WiFi] Retry NTP Sync...");
                    if (syncTimeWithNTP()) {
                        setTimeSyncStatus(true);
                    }
                }
            }
        }

        // --- 2. GESTION MQTT ---
        if (WiFi.status() == WL_CONNECTED) {
            if (!mqttClient.connected()) {
                setMqttStatus(false);
                Serial.println("[MQTT] Connexion au broker...");
                
                if (mqttClient.connect(MQTT_SERVER, MQTT_PORT)) {
                    Serial.println("[MQTT] Connecté !");
                    setMqttStatus(true);
                } else {
                    Serial.print("[MQTT] Échec, code d'erreur=");
                    Serial.println(mqttClient.connectError());
                    // Délai avant retry MQTT
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                }
            }
            
            if (mqttClient.connected()) {
                setMqttStatus(true);
                mqttClient.poll();

                // --- 3. PUBLICATION ---
                SystemData data = getSystemData();

                // Publication Locale (Cafeteria)
                if (!isnan(data.localTemperature)) {
                    String payload = "{";
                    payload += "\"source\":\"cafeteria\",";
                    payload += "\"temperature\":" + String(data.localTemperature) + ",";
                    payload += "\"humidity\":" + String(data.localHumidity) + ",";
                    payload += "\"loraStatus\":" + String(data.loraModuleConnected ? "true" : "false") + ",";
                    payload += "\"wifiStatus\":true,";
                    payload += "\"mqttStatus\":true";
                    payload += "}";
                    
                    // Publication avec QoS 1 (At Least Once)
                    mqttClient.beginMessage(MQTT_TOPIC_CAFET, false, 1);
                    mqttClient.print(payload);
                    if (mqttClient.endMessage()) {
                         // Publication réussie et confirmée par le broker
                    } else {
                         Serial.println("[MQTT] Erreur lors de la publication Cafet (QoS 1 non confirmé)");
                    }
                }

                // Publication Distante (Fablab)
                if (!isnan(data.fablab.temperature)) {
                    String payload = "{";
                    payload += "\"source\":\"fablab\",";
                    payload += "\"temperature\":" + String(data.fablab.temperature) + ",";
                    payload += "\"humidity\":" + String(data.fablab.humidity) + ",";
                    payload += "\"lastUpdate\":" + String(millis() - data.fablab.lastUpdate);
                    payload += "}";
                    
                    // Publication avec QoS 1
                    mqttClient.beginMessage(MQTT_TOPIC_FABLAB, false, 1);
                    mqttClient.print(payload);
                    if (!mqttClient.endMessage()) {
                        Serial.println("[MQTT] Erreur lors de la publication Fablab (QoS 1 non confirmé)");
                    }
                }
            }
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
