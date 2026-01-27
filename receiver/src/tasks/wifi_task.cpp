#include "wifi_task.h"
#include <WiFiS3.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../web/web_assets.h"
#include "../utils/data_manager.h"
#include "../utils/led_matrix_manager.h"

WiFiServer server(80);

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
                
                // Affichage sur la matrice (texte défilant non bloquant géré par le driver R4 normalement, 
                // mais ici on l'appelle périodiquement)
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
            Serial.println("\n[WiFi] Connecté ! IP: ");
            Serial.println(WiFi.localIP());
            server.begin();
            
            // Une fois connecté, on éteint la matrice (ou on pourrait afficher un smiley)
            clearLedMatrix();
        }

        // --- 2. SERVEUR WEB ---
        WiFiClient client = server.available();

        if (client) {
            boolean isApiRequest = false;
            boolean currentLineIsBlank = true;
            String currentLine = "";
            boolean firstLine = true;

            while (client.connected()) {
                if (client.available()) {
                    char c = client.read();

                    if (firstLine && currentLine.length() < 64) { 
                        if (c != '\r' && c != '\n') {
                            currentLine += c;
                        }
                        if (c == '\n') {
                            if (currentLine.indexOf("GET /api/data") >= 0) {
                                isApiRequest = true;
                            }
                            firstLine = false;
                            currentLine = "";
                        }
                    } 
                    else {
                        if (c == '\n' && currentLineIsBlank) {
                            
                            if (isApiRequest) {
                                SystemData data = getSystemData();

                                client.println("HTTP/1.1 200 OK");
                                client.println("Content-Type: application/json");
                                client.println("Connection: close");
                                client.println("Access-Control-Allow-Origin: *");
                                client.println();
                                
                                client.print("{");
                                client.print("\"localTemp\":"); client.print(data.localTemperature); client.print(",");
                                client.print("\"localHum\":"); client.print(data.localHumidity); client.print(",");
                                client.print("\"remoteTemp\":"); client.print(data.remoteTemperature); client.print(",");
                                client.print("\"remoteHum\":"); client.print(data.remoteHumidity); client.print(",");
                                client.print("\"lastUpdate\":"); client.print(millis() - data.lastLoRaUpdate); client.print(",");
                                
                                // Ajout des status d'erreurs
                                client.print("\"loraStatus\":"); client.print(data.loraModuleConnected ? "true" : "false"); client.print(",");
                                client.print("\"dhtStatus\":"); client.print(data.dhtModuleConnected ? "true" : "false");
                                
                                client.print("}");
                                
                            } else {
                                client.println("HTTP/1.1 200 OK");
                                client.println("Content-Type: text/html; charset=UTF-8");
                                client.println("Connection: close");
                                client.println();
                                client.print(INDEX_HTML);
                            }
                            break; 
                        }
                        
                        if (c == '\n') {
                            currentLineIsBlank = true;
                        } else if (c != '\r') {
                            currentLineIsBlank = false;
                        }
                    }
                }
            }
            delay(10); 
            client.stop();
        }
        
        // Vérification périodique connexion (toutes les 100ms)
        delay(100);
    }
}
