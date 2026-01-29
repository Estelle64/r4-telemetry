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
const char* mqttTopic = "home/cafet/temp";

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
                displayTemperatureOnMatrix((int)data.localTemperature);

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

            String payload = "{";
            payload += "\"localTemp\":" + String(data.localTemperature) + ",";
            payload += "\"localHum\":" + String(data.localHumidity) + ",";
            payload += "\"remoteTemp\":" + String(data.remoteTemperature) + ",";
            payload += "\"remoteHum\":" + String(data.remoteHumidity) + ",";
            payload += "\"lastUpdate\":" + String(millis() - data.lastLoRaUpdate) + ",";
            payload += "\"loraStatus\":" + String(data.loraModuleConnected ? "true" : "false") + ",";
            payload += "\"dhtStatus\":" + String(data.dhtModuleConnected ? "true" : "false") + ",";
            payload += "\"timeSynced\":" + String(data.timeSynced ? "true" : "false");
            payload += "}";

            mqttClient.publish(mqttTopic, payload.c_str());
        }

        // Maintenir MQTT actif
        mqttClient.loop();

        // Mise à jour affichage LED
        SystemData displayData = getSystemData();
        displayTemperatureOnMatrix((int)displayData.localTemperature);

        // Pause 2 secondes avant prochaine publication
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
