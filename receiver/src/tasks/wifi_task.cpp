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
    
    if (topic == MQTT_TOPIC_HANDSHAKE_RES) {
        String payload = "";
        while (mqttClient.available()) {
            payload += (char)mqttClient.read();
        }
        
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc.containsKey("seq")) {
            uint32_t seq = doc["seq"];
            Serial.print("[MQTT] Handshake réussi ! Séquence initiale : ");
            Serial.println(seq);
            setMqttSequence(seq);
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
            setMqttHandshakeDone(false); // Reset handshake si on perd la connexion
            
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
                setMqttHandshakeDone(false);
                Serial.println("[MQTT] Connexion au broker...");
                
                if (mqttClient.connect(MQTT_SERVER, MQTT_PORT)) {
                    Serial.println("[MQTT] Connecté !");
                    setMqttStatus(true);
                    
                    // Souscription au topic de handshake
                    mqttClient.subscribe(MQTT_TOPIC_HANDSHAKE_RES);
                    
                    // Demande de handshake
                    mqttClient.beginMessage(MQTT_TOPIC_HANDSHAKE_REQ);
                    mqttClient.print("{\"id\":\"cafeteria\"}");
                    mqttClient.endMessage();
                    Serial.println("[MQTT] Handshake demandé...");
                } else {
                    Serial.print("[MQTT] Échec, code=");
                    Serial.println(mqttClient.connectError());
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                }
            }
            
            if (mqttClient.connected()) {
                setMqttStatus(true);
                mqttClient.poll();

                // On ne publie les données que si le handshake est fait
                if (isMqttHandshakeDone()) {
                    SystemData data = getSystemData();

                    // Publication Locale (Cafeteria)
                    if (!isnan(data.localTemperature)) {
                        uint32_t seq = getNextMqttSequence();
                        
                        StaticJsonDocument<512> doc;
                        // On force 1 seule décimale pour éviter les écarts de stringify
                        doc["humidity"] = serialized(String(data.localHumidity, 1));
                        doc["loraStatus"] = data.loraModuleConnected;
                        doc["seq"] = seq;
                        doc["source"] = "cafeteria";
                        doc["temperature"] = serialized(String(data.localTemperature, 1));
                        
                        // Création de la signature HMAC sur le JSON compact (clés triées)
                        String payload;
                        serializeJson(doc, payload);
                        
                        Serial.print("[Security] String to sign: ");
                        Serial.println(payload);

                        uint8_t hmac_res[32];
                        hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), 
                                    (const uint8_t*)payload.c_str(), payload.length(), hmac_res);
                        
                        doc["hmac"] = hashToString(hmac_res);
                        
                        payload = "";
                        serializeJson(doc, payload);
                        
                        mqttClient.beginMessage(MQTT_TOPIC_CAFET, false, 1);
                        mqttClient.print(payload);
                        if (!mqttClient.endMessage()) {
                             Serial.println("[MQTT] Erreur publication Cafet (QoS 1)");
                        }
                    }
                }
            }
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
