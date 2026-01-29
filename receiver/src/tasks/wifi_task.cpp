#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/led_matrix_manager.h"
#include "../utils/time_manager.h"

// Pas de serveur Web pour le moment (MQTT plus tard)
// WiFiServer server(80);

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
                
                // Affichage sur la matrice
                displayTemperatureOnMatrix(tempToShow);

                delay(500); 
                Serial.print(".");
                tryCount++;
                
                // Si au bout de 20 essais (10s) toujours rien, on réessaie begin
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
        }

        // --- 2. MAINTIEN / FUTUR MQTT ---
        // Ici, on pourra ajouter la logique MQTT
        
        // Vérification périodique connexion (toutes les 1s)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
