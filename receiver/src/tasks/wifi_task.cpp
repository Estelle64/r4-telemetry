#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include <PubSubClient.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/led_matrix_manager.h"
#include "../utils/time_manager.h"

// ---------------------- MQTT Setup ----------------------
WiFiClient espClient;
PubSubClient mqttClient(espClient);
const char* mqttServer = "172.20.10.12"; // IP du broker central
const int mqttPort = 1883;
const char* mqttCafetTopic = "cesi/cafet";
const char* mqttFablabTopic = "cesi/fablab";

// ---------------------- Task ----------------------
void wifiTask(void *pvParameters) {
    // Initialisation de la matrice LED
    initLedMatrix();

    for (;;) {
        // --- 1. CONNEXION WIFI ---
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Tentative de connexion...");
            WiFi.begin(WIFI_SSID, WIFI_PASS);

            int tryCount = 0;
            while (WiFi.status() != WL_CONNECTED) {
                SystemData data = getSystemData();
                if (!isnan(data.localTemperature)) {
                    displayTemperatureOnMatrix((int)data.localTemperature);
                }

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

            clearLedMatrix();

            // --- 2. CONNEXION MQTT ---
            mqttClient.setServer(mqttServer, mqttPort);
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

            // Publish Cafeteria Data
            if (!isnan(data.cafeteria.temperature)) {
                String payload = "{";
                payload += "\"source\":\"cafeteria\",";
                payload += "\"localTemp\":" + String(data.localTemperature) + ",";
                payload += "\"localHum\":" + String(data.localHumidity) + ",";
                payload += "\"remoteTemp\":" + String(data.cafeteria.temperature) + ",";
                payload += "\"remoteHum\":" + String(data.cafeteria.humidity) + ",";
                payload += "\"lastUpdate\":" + String(millis() - data.cafeteria.lastUpdate) + ",";
                payload += "\"loraStatus\":" + String(data.loraModuleConnected ? "true" : "false") + ",";
                payload += "\"dhtStatus\":" + String(data.dhtModuleConnected ? "true" : "false") + ",";
                payload += "\"timeSynced\":" + String(data.timeSynced ? "true" : "false");
                payload += "}";
                mqttClient.publish(mqttCafetTopic, payload.c_str());
                Serial.println("[MQTT] Published Cafeteria data.");
            }

            // Publish Fablab Data
            if (!isnan(data.fablab.temperature)) {
                String payload = "{";
                payload += "\"source\":\"fablab\",";
                payload += "\"localTemp\":" + String(data.localTemperature) + ",";
                payload += "\"localHum\":" + String(data.localHumidity) + ",";
                payload += "\"remoteTemp\":" + String(data.fablab.temperature) + ",";
                payload += "\"remoteHum\":" + String(data.fablab.humidity) + ",";
                payload += "\"lastUpdate\":" + String(millis() - data.fablab.lastUpdate) + ",";
                payload += "\"loraStatus\":" + String(data.loraModuleConnected ? "true" : "false") + ",";
                payload += "\"dhtStatus\":" + String(data.dhtModuleConnected ? "true" : "false") + ",";
                payload += "\"timeSynced\":" + String(data.timeSynced ? "true" : "false");
                payload += "}";
                mqttClient.publish(mqttFablabTopic, payload.c_str());
                Serial.println("[MQTT] Published Fablab data.");
            }
        }

        // Maintenir MQTT actif
        mqttClient.loop();

        // Mise à jour affichage LED
        SystemData displayData = getSystemData();
        if (!isnan(displayData.localTemperature)) {
            displayTemperatureOnMatrix((int)displayData.localTemperature);
        }

        // Pause 2 secondes avant prochaine publication
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
