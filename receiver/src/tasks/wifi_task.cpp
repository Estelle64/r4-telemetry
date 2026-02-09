#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/time_manager.h"
#include "../utils/security_utils.h"

// ---------------------- MQTT Setup ----------------------
WiFiClient espClient;
MqttClient mqttClient(espClient);

// Callback pour les messages MQTT (Handshake)
void onMqttMessage(int messageSize) {
    String topic = mqttClient.messageTopic();
    
    if (topic.startsWith("cesi/handshake/res/")) {
        String payload = "";
        while (mqttClient.available()) {
            payload += (char)mqttClient.read();
        }
        
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc.containsKey("seq") && doc.containsKey("id")) {
            const char* id = doc["id"];
            uint32_t seq = doc["seq"];
            Serial.print("[MQTT] Handshake réussi pour ");
            Serial.print(id);
            Serial.print(" ! Séquence : ");
            Serial.println(seq);
            setMqttSequence(id, seq);
        }
    }
}

// ---------------------- Task ----------------------
void wifiTask(void *pvParameters) {
    // Identification du client
    mqttClient.setId("ArduinoPasserelle");
    mqttClient.onMessage(onMqttMessage);

    for (;;) {
        // --- 1. GESTION WIFI ---
        // (Code de connexion WiFi inchangé...)
        if (WiFi.status() != WL_CONNECTED) {
            setWifiStatus(false);
            setMqttStatus(false);
            setMqttHandshakeDone("cafeteria", false); 
            setMqttHandshakeDone("fablab", false); // Reset handshake si on perd la connexion
            
            Serial.println("[WiFi] Connexion perdue ou non établie. Tentative...");
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            
            int tryCount = 0;
            while (WiFi.status() != WL_CONNECTED && tryCount < 20) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                Serial.print(".");
                tryCount++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\n[WiFi] Connecté ! IP: " + WiFi.localIP().toString());
                setWifiStatus(true);
                if (syncTimeWithNTP()) setTimeSyncStatus(true);
            } else {
                 Serial.println("\n[WiFi] Échec. Prochaine tentative dans 5s...");
                 vTaskDelay(5000 / portTICK_PERIOD_MS);
                 continue;
            }
        } else {
            setWifiStatus(true);
        }

        // --- 2. GESTION MQTT ---
        if (WiFi.status() == WL_CONNECTED) {
            if (!mqttClient.connected()) {
                setMqttStatus(false);
                setMqttHandshakeDone("cafeteria", false);
                setMqttHandshakeDone("fablab", false);
                Serial.println("[MQTT] Connexion au broker...");
                
                if (mqttClient.connect(MQTT_SERVER, MQTT_PORT)) {
                    Serial.println("[MQTT] Connecté !");
                    setMqttStatus(true);
                    
                    // Souscription aux topics de handshake (Wildcard pour les deux)
                    mqttClient.subscribe("cesi/handshake/res/#");
                    
                    // Demandes initiales
                    mqttClient.beginMessage(MQTT_TOPIC_HANDSHAKE_REQ);
                    mqttClient.print("{\"id\":\"cafeteria\"}");
                    mqttClient.endMessage();
                    
                    mqttClient.beginMessage(MQTT_TOPIC_HANDSHAKE_REQ);
                    mqttClient.print("{\"id\":\"fablab\"}");
                    mqttClient.endMessage();
                    
                    Serial.println("[MQTT] Handshakes demandés (Cafet & Fablab)...");
                } else {
                    Serial.print("[MQTT] Échec, code=");
                    Serial.println(mqttClient.connectError());
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                }
            }
            
            if (mqttClient.connected()) {
                setMqttStatus(true);
                mqttClient.poll();

                // Retry Handshake si pas fait
                static unsigned long lastHandshakeRetry = 0;
                if (millis() - lastHandshakeRetry > 10000) {
                    lastHandshakeRetry = millis();
                    if (!isMqttHandshakeDone("cafeteria")) {
                        mqttClient.beginMessage(MQTT_TOPIC_HANDSHAKE_REQ);
                        mqttClient.print("{\"id\":\"cafeteria\"}");
                        mqttClient.endMessage();
                    }
                    if (!isMqttHandshakeDone("fablab")) {
                        mqttClient.beginMessage(MQTT_TOPIC_HANDSHAKE_REQ);
                        mqttClient.print("{\"id\":\"fablab\"}");
                        mqttClient.endMessage();
                    }
                }

                SystemData data = getSystemData();

                // 1. Publication Locale (Cafeteria)
                if (isMqttHandshakeDone("cafeteria") && !isnan(data.localTemperature)) {
                    uint32_t seq = getNextMqttSequence("cafeteria");
                    StaticJsonDocument<512> doc;
                    doc["humidity"] = String(data.localHumidity, 1);
                    doc["loraStatus"] = data.loraModuleConnected;
                    
                    // Diagnostics LoRa (si reçus via LoRa)
                    if (data.cafeteria.packetsReceived > 0) {
                        doc["packetsLost"] = data.cafeteria.packetsLost;
                        doc["packetsReceived"] = data.cafeteria.packetsReceived;
                        doc["rssi"] = data.cafeteria.rssi;
                    }
                    
                    doc["seq"] = seq;
                    
                    if (data.cafeteria.packetsReceived > 0) {
                        doc["snr"] = data.cafeteria.snr;
                    }

                    doc["source"] = "cafeteria";
                    doc["temperature"] = String(data.localTemperature, 1);
                    
                    String payload;
                    serializeJson(doc, payload);
                    uint8_t hmac_res[32];
                    hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), 
                                (const uint8_t*)payload.c_str(), payload.length(), hmac_res);
                    doc["hmac"] = hashToString(hmac_res);
                    payload = "";
                    serializeJson(doc, payload);
                    
                    mqttClient.beginMessage(MQTT_TOPIC_CAFET, false, 1);
                    mqttClient.print(payload);
                    mqttClient.endMessage();
                }

                // 2. Publication Distante (Fablab)
                if (isMqttHandshakeDone("fablab") && !isnan(data.fablab.temperature)) {
                    uint32_t seq = getNextMqttSequence("fablab");
                    StaticJsonDocument<512> doc;
                    doc["humidity"] = String(data.fablab.humidity, 1);
                    doc["loraStatus"] = data.loraModuleConnected;
                    doc["packetsLost"] = data.fablab.packetsLost;
                    doc["packetsReceived"] = data.fablab.packetsReceived;
                    doc["rssi"] = data.fablab.rssi;
                    doc["seq"] = seq;
                    doc["snr"] = data.fablab.snr;
                    doc["source"] = "fablab";
                    doc["temperature"] = String(data.fablab.temperature, 1);
                    
                    String payload;
                    serializeJson(doc, payload);
                    uint8_t hmac_res[32];
                    hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), 
                                (const uint8_t*)payload.c_str(), payload.length(), hmac_res);
                    doc["hmac"] = hashToString(hmac_res);
                    payload = "";
                    serializeJson(doc, payload);
                    
                    mqttClient.beginMessage(MQTT_TOPIC_FABLAB, false, 1);
                    mqttClient.print(payload);
                    mqttClient.endMessage();
                }
            }
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
