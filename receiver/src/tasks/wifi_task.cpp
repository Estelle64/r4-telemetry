#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include <PubSubClient.h> // Ajout de la lib MQTT
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/led_matrix_manager.h"
#include "../utils/time_manager.h"

// ---------------------- MQTT Setup (Merged from colleague) ----------------------
WiFiClient espClient;
PubSubClient mqttClient(espClient);
const char* mqttServer = "172.20.10.7"; // IP du broker central
const int mqttPort = 1883;
const char* mqttTopic = "home/cafet/temp";

void wifiTask(void *pvParameters) {
    // Init Matrix
    initLedMatrix();

    // Boucle infinie de reconnexion et service
    for (;;) {
        
        // --- 1. GESTION CONNEXION WIFI ---
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Tentative de connexion...");
            WiFi.begin(WIFI_SSID, WIFI_PASS);
            
            int tryCount = 0;
            while (WiFi.status() != WL_CONNECTED) {
                // Si pas connecté, on affiche la température locale sur la matrice
                SystemData data = getSystemData();
                int tempToShow = (int)data.localTemperature;
                displayTemperatureOnMatrix(tempToShow);

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
            
            // Tentative de synchronisation du temps
            if (syncTimeWithNTP()) {
                setTimeSyncStatus(true);
            }

            // Une fois connecté, on éteint la matrice
            clearLedMatrix();
            
            // --- 2. CONFIG MQTT (Après connexion WiFi) ---
            mqttClient.setServer(mqttServer, mqttPort);
        }

        // --- 3. GESTION CONNEXION MQTT ---
        if (!mqttClient.connected()) {
            Serial.println("[MQTT] Tentative de connexion...");
            // ID Client aléatoire ou fixe
            if (mqttClient.connect("ArduinoPasserelle")) {
                Serial.println("[MQTT] Connecté !");
            } else {
                Serial.print("[MQTT] Échec, rc=");
                Serial.print(mqttClient.state());
                Serial.println(" (Retrying in 2s)");
                vTaskDelay(pdMS_TO_TICKS(2000));
                continue; // On retourne au début pour revérifier le WiFi si besoin
            }
        }

        // --- 4. PUBLICATION DES DONNÉES ---
        if (mqttClient.connected()) {
            mqttClient.loop(); // Important pour maintenir la connexion et recevoir des messages

            SystemData data = getSystemData();

            // Construction du JSON (Merci à la collègue)
            String payload = "{";
            payload += "\"localTemp\":