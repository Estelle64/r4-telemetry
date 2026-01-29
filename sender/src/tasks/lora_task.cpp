#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/sleep_manager.h"
#include "../utils/crypto_manager.h"

// --- CONSTANTES ---
const int LED_PIN = 13;

// Plus de lastHash chainé.
// static uint8_t lastHash[32]; 

// --- FONCTIONS UTILITAIRES ---

static String byteToHexString(uint8_t b) {
    String s = String(b, HEX);
    if (s.length() < 2) s = "0" + s;
    return s;
}

static void sendAT(String cmd) {
    Serial1.print(cmd + "\r\n");
}

static String readResponse(uint16_t timeout = 2000) {
    String s = "";
    uint32_t t0 = millis();
    while (millis() - t0 < timeout) {
        while (Serial1.available()) {
            char c = Serial1.read();
            s += c;
        }
        delay(10);
    }
    return s;
}

// Fonction pour envoyer un paquet combiné avec HMAC
// Format Payload (Raw Bytes):
// [0] SENSOR_ID
// [1] TYPE (2 = Mixed Temp+Hum)
// [2-3] Temp (int16_t * 100)
// [4-5] Hum (int16_t * 100)
// [6-37] HMAC (32 bytes)
// Total: 38 bytes -> 76 hex chars
static bool sendSecurePacket(float temp, float hum) {
    uint8_t payload[6]; // Données brutes
    int16_t tInt = (int16_t)(temp * 100);
    int16_t hInt = (int16_t)(hum * 100);

    payload[0] = SENSOR_ID;
    payload[1] = 2; // Type 2 = Mixed
    payload[2] = (uint8_t)(tInt & 0xFF);
    payload[3] = (uint8_t)((tInt >> 8) & 0xFF);
    payload[4] = (uint8_t)(hInt & 0xFF);
    payload[5] = (uint8_t)((hInt >> 8) & 0xFF);

    // 1. Calcul du HMAC sur le Payload SEUL (Authenticité simple)
    uint8_t newHash[32];
    CryptoManager::hmac_sha256(
        (const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), 
        payload, 6, 
        newHash
    );

    // 2. Construction du paquet final HexString
    String hexPayload = "";
    
    // Ajout Data
    for(int i=0; i<6; i++) hexPayload += byteToHexString(payload[i]);
    
    // Ajout Hash
    for(int i=0; i<32; i++) hexPayload += byteToHexString(newHash[i]);

    Serial.println("[LoRa] Sending Secure Packet...");
    Serial.print("   Hash: "); Serial.println(CryptoManager::hashToString(newHash, 32));

    // 3. Envoi
    while(Serial1.available()) Serial1.read();

    sendAT("AT+TEST=TXLRPKT,\"" + hexPayload + "\"");
    
    // ATTENTE OPTIMISÉE DU TX DONE
    uint32_t txStart = millis();
    String txResp = "";
    while(millis() - txStart < 2000) {
        while(Serial1.available()) {
            char c = Serial1.read();
            txResp += c;
        }
        if (txResp.indexOf("TX DONE") != -1) {
            break; 
        }
        delay(5);
    }
    Serial.println("[LoRa] TX Finished. Switching to RX for ACK.");

    // 4. Attente ACK
    Serial.println("[LoRa] Waiting for ACK...");
    sendAT("AT+TEST=RXLRPKT");
    
    String ackBuffer = "";
    uint32_t tStart = millis();
    bool ackReceived = false;

    // On écoute pendant 5 secondes max
    while(millis() - tStart < 5000) {
        if (Serial1.available()) {
            char c = Serial1.read();
            ackBuffer += c;
            
            if (c == '\n') {
                if (ackBuffer.indexOf("+TEST: RX \"") != -1) { 
                    int firstQuote = ackBuffer.indexOf('"');
                    int lastQuote = ackBuffer.lastIndexOf('"');
                    if (firstQuote != -1 && lastQuote > firstQuote) {
                        String hexAck = ackBuffer.substring(firstQuote + 1, lastQuote);
                        
                        // ACK = Le hash qu'on vient d'envoyer
                        if (hexAck.length() >= 64) {
                            String targetHashStr = CryptoManager::hashToString(newHash, 32);
                            targetHashStr.toUpperCase();
                            hexAck.toUpperCase();

                            if (hexAck.indexOf(targetHashStr) != -1) {
                                Serial.println("[LoRa] ACK VALIDATED ! Packet delivered.");
                                ackReceived = true;
                                break;
                            }
                        }
                    }
                }
                ackBuffer = "";
            }
        }
        delay(1); 
    }

    if (!ackReceived) {
        Serial.println("[LoRa] ACK Timeout. Packet may be lost.");
        return false;
    }

    return true;
}

// --- TÂCHE PRINCIPALE ---

void loraTask(void *pvParameters) {
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    Serial1.begin(9600); 

    Serial.println("[LoRaTask] Config module...");
    sendAT("AT+MODE=TEST");
    delay(200);
    readResponse();
    // Config: 868MHz, SF7, BW125
    sendAT("AT+TEST=RFCFG,868,SF7,125,12,15,14");
    delay(200);
    readResponse();
    
    Serial.println("[LoRaTask] Prêt.");

    for (;;) {
        SensorData data = getSensorData();

        if (data.valid) {
            digitalWrite(LED_PIN, HIGH);
            
            // Tentative d'envoi sécurisé
            bool success = sendSecurePacket(data.temperature, data.humidity);
            
            digitalWrite(LED_PIN, LOW);

            if (success) {
                // Si succès, on dort longtemps
                Serial.println("[LoRaTask] Success -> Deep Sleep.");
                SleepManager::deepSleep(DEEP_SLEEP_INTERVAL_SEC);
            } else {
                // Si échec, on réessaie plus vite (ou on dort quand même, au choix)
                // Ici on décide de dormir quand même pour économiser la batterie, le prochain paquet sera une nouvelle donnée.
                Serial.println("[LoRaTask] Failed -> Retry Sleep.");
                // SleepManager::deepSleep(60); // Version courte si on voulait
                SleepManager::deepSleep(DEEP_SLEEP_INTERVAL_SEC); 
            }

            Serial.println("[LoRaTask] Woke up.");
            
            Serial1.end();
            Serial1.begin(9600);
            
            Serial.println("[LoRaTask] Waiting for fresh sensor data...");
            delay(5000); 
            
            Serial.println("[LoRaTask] Ready to send next packet.");

        } else {
            Serial.println("[LoRaTask] No valid data. Retrying...");
            delay(2000);
        }
    }
}
