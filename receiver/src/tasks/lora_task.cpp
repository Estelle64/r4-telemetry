#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/crypto_manager.h"

// --- CONSTANTES & DEFINITIONS ---
const int LED_PIN = 13;

// --- FONCTIONS UTILITAIRES ---

static String doubleHexToHexString(const String &doubleHex) {
    String result = "";
    result.reserve(doubleHex.length() / 2);
    for (size_t i = 0; i < doubleHex.length(); i += 2) {
        String byteStr = doubleHex.substring(i, i + 2);
        char byteChar = (char)strtol(byteStr.c_str(), NULL, 16);
        result += byteChar;
    }
    return result;
}

static void hexStringToBytes(const String &hexString, uint8_t *bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        String byteStr = hexString.substring(i*2, i*2+2);
        char byteChar[3] = {byteStr[0], byteStr[1], '\0'};
        bytes[i] = (uint8_t)strtol(byteChar, NULL, 16);
    }
}

static void sendAT(String cmd) {
    Serial1.print(cmd + "\r\n");
}

static String readResponse(uint16_t timeout = 500) {
    String s = "";
    uint32_t t0 = millis();
    while (millis() - t0 < timeout) {
        while (Serial1.available()) {
            char c = Serial1.read();
            s += c;
        }
        delay(1); 
    }
    return s;
}

static void sendAck(const uint8_t *hashToSend) {
    String hashStr = CryptoManager::hashToString(hashToSend, 32);
    Serial.println("[LoRa] Sending ACK: " + hashStr);
    
    // On envoie le hash comme ACK pour confirmer ce paquet spécifique
    sendAT("AT+TEST=TXLRPKT,\"" + hashStr + "\"");
    
    delay(200); 
    
    // On se remet immédiatement en écoute
    sendAT("AT+TEST=RXLRPKT");
}

// --- TÂCHE PRINCIPALE (Simplified HMAC Check) ---

static void processRxLine(String line) {
    int rxIndex = line.indexOf("+TEST: RX \"");
    if (rxIndex == -1) return;

    Serial.println("[LoRa] Packet Detected.");

    int firstQuote = line.indexOf('"', rxIndex);
    int lastQuote = line.lastIndexOf('"');

    if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
        String hexContent = line.substring(firstQuote + 1, lastQuote);

        if (hexContent.length() == 76) { // 38 bytes = 6 bytes Data + 32 bytes HMAC
            uint8_t payload[38];
            hexStringToBytes(hexContent, payload, 38);

            uint8_t dataPart[6];
            uint8_t receivedHash[32];
            
            memcpy(dataPart, payload, 6);
            memcpy(receivedHash, payload + 6, 32);

            // --- VERIFICATION HMAC SIMPLE ---
            // On vérifie que HMAC(Secret, Data) == HashReçu
            // Pas de chaining avec lastHash
            
            uint8_t calculatedHash[32];
            CryptoManager::hmac_sha256(
                (const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET),
                dataPart, 6, // On signe uniquement les données
                calculatedHash
            );

            if (memcmp(receivedHash, calculatedHash, 32) == 0) {
                Serial.println("[LoRa] VALID PACKET (Authenticated).");
                
                uint8_t sensorId = dataPart[0];
                int16_t tRaw = (int16_t)(dataPart[2] | (dataPart[3] << 8));
                int16_t hRaw = (int16_t)(dataPart[4] | (dataPart[5] << 8));

                Serial.print("[LoRa] Data from Sensor ID: ");
                Serial.println(sensorId);

                updateRemoteData(sensorId, tRaw / 100.0, hRaw / 100.0);
                
                digitalWrite(LED_PIN, HIGH);
                delay(50);
                digitalWrite(LED_PIN, LOW);

                // ACK pour dire "J'ai bien reçu ce paquet"
                sendAck(receivedHash);
                return;

            } else {
                Serial.println("[LoRa] ERROR: Invalid Signature (HMAC mismatch).");
            }
        } else {
             Serial.println("[LoRa] Ignored: Invalid length (" + String(hexContent.length()) + ")");
        }
    }
    
    Serial.println("[LoRa] Re-arming RX mode (Invalid Packet).");
    delay(100);
    sendAT("AT+TEST=RXLRPKT");
}

void loraTask(void *pvParameters) {
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    Serial1.begin(9600); 
    
    Serial.println("[LoRaTask] Config...");
    
    sendAT("AT+MODE=TEST");
    delay(100); readResponse();
    
    sendAT("AT+TEST=RFCFG,868,SF7,125,12,15,14");
    delay(100); readResponse();
    
    // First arming
    sendAT("AT+TEST=RXLRPKT");
    
    Serial.println("[LoRaTask] Ready.");

    String lineBuffer = "";

    for (;;) {
        while (Serial1.available()) {
            char c = Serial1.read();
            if (c == '\n') {
                lineBuffer.trim(); 
                if (lineBuffer.length() > 0) {
                    Serial.println("[LoRa RAW] " + lineBuffer);
                    if (lineBuffer.indexOf("+TEST: RX") != -1) {
                        processRxLine(lineBuffer);
                    }
                }
                lineBuffer = "";
            } else {
                lineBuffer += c;
            }
        }
        vTaskDelay(10); 
    }
}