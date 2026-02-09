#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <RTC.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/sleep_manager.h"
#include "../utils/security_utils.h"

// --- CONFIGURATION ---
const int LED_PIN = 13;
const int LORA_BAUD_RATE = 9600;

// --- UTILS ---

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
            s += (char)Serial1.read();
        }
        delay(10);
    }
    return s;
}

static bool initLoRaModule() {
    Serial.println("[LoRa] Initialisation du module...");
    Serial1.begin(LORA_BAUD_RATE);
    
    // Retry Loop
    for (int i = 0; i < 3; i++) {
        while(Serial1.available()) Serial1.read(); // Clear buffer
        
        sendAT("AT");
        String resp = readResponse(1000);
        
        if (resp.indexOf("OK") != -1) {
             Serial.println("[LoRa] Module OK.");
             // Config
             sendAT("AT+MODE=TEST"); delay(100); readResponse();
             sendAT("AT+TEST=RFCFG,868,SF7,125,12,15,14"); delay(100); readResponse();
             return true;
        }
        Serial.println("[LoRa] Pas de réponse... Retry.");
        delay(500);
    }
    Serial.println("[LoRa] Echec connexion module.");
    return false;
}

// --- LORA LOGIC ---

// Request Time from Receiver
// Packet: [ID, TYPE=3, 0,0,0,0 (Padding), HMAC]
static bool requestTimeSync() {
    Serial.println("[LoRa] Requesting Time Sync...");
    
    // 1. Prepare Data
    uint8_t payload[6]; 
    payload[0] = SENSOR_ID;
    payload[1] = 3; // Type 3 = Time Request
    memset(&payload[2], 0, 4); // Padding

    // 2. Sign Data (HMAC)
    uint8_t hmac[32];
    hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), payload, 6, hmac);

    // 3. Build Hex String
    String hexPayload = "";
    for(int i=0; i<6; i++) hexPayload += byteToHexString(payload[i]);
    for(int i=0; i<32; i++) hexPayload += byteToHexString(hmac[i]);

    // 4. Send
    while(Serial1.available()) Serial1.read();
    sendAT("AT+TEST=TXLRPKT,\"" + hexPayload + "\"");

    // 5. Wait TX DONE
    uint32_t tStart = millis();
    bool txDone = false;
    while(millis() - tStart < 3000) {
        String line = Serial1.readStringUntil('\n');
        if (line.indexOf("TX DONE") != -1) {
            txDone = true; break;
        }
    }
    if(!txDone) { Serial.println("[LoRa] TX Error during sync."); return false; }

    // 6. Wait for Response (Type 4)
    Serial.println("[LoRa] Waiting for Time Response...");
    sendAT("AT+TEST=RXLRPKT");
    
    String rxBuffer = "";
    tStart = millis();
    while(millis() - tStart < 3000) { // Reduced timeout to 3s to be less blocking
        while(Serial1.available()) {
            char c = Serial1.read();
            rxBuffer += c;
            if (c == '\n') {
                if (rxBuffer.indexOf("+TEST: RX \"") != -1) {
                    int first = rxBuffer.indexOf('"');
                    int last = rxBuffer.lastIndexOf('"');
                    if(first != -1 && last > first) {
                        String hexContent = rxBuffer.substring(first+1, last);
                        if(hexContent.length() == 76) {
                            // Decode (Simplified for sync check)
                            uint8_t packet[38];
                            for(int i=0; i<38; i++) {
                                String b = hexContent.substring(i*2, i*2+2);
                                packet[i] = (uint8_t)strtol(b.c_str(), NULL, 16);
                            }
                            
                            // Check Type
                            if(packet[1] == 4) { // Time Response
                                uint32_t receivedTime = packet[2] | (packet[3] << 8) | (packet[4] << 16) | (packet[5] << 24);
                                Serial.print("[LoRa] Time Sync Success! Unix Time: ");
                                Serial.println(receivedTime);
                                
                                RTCTime timeToSet(receivedTime);
                                RTC.setTime(timeToSet);
                                return true; 
                            }
                        }
                    }
                }
                rxBuffer = "";
            }
        }
    }
    Serial.println("[LoRa] Time Sync Failed (Timeout).");
    return false;
}

// Send Packet: [ID, Type, Seq, T_low, T_high, H_low, H_high] + [HMAC(32)]
static bool sendSecurePacket(float temp, float hum) {
    static uint8_t sequenceCounter = 0;
    
    // 1. Prepare Data
    uint8_t payload[7]; 
    
    // Valeur magique 0x7FFF si le capteur est en erreur (NAN)
    int16_t tInt = isnan(temp) ? 0x7FFF : (int16_t)(temp * 100);
    int16_t hInt = isnan(hum) ? 0x7FFF : (int16_t)(hum * 100);

    payload[0] = SENSOR_ID;
    payload[1] = 2; // Type 2 = Mixed
    payload[2] = sequenceCounter++;
    payload[3] = (uint8_t)(tInt & 0xFF);
    payload[4] = (uint8_t)((tInt >> 8) & 0xFF);
    payload[5] = (uint8_t)(hInt & 0xFF);
    payload[6] = (uint8_t)((hInt >> 8) & 0xFF);

    // 2. Sign Data (HMAC)
    uint8_t hmac[32];
    hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), payload, 7, hmac);

    // 3. Build Hex String
    String hexPayload = "";
    for(int i=0; i<7; i++) hexPayload += byteToHexString(payload[i]);
    for(int i=0; i<32; i++) hexPayload += byteToHexString(hmac[i]);

    String sentHashStr = hashToString(hmac);
    sentHashStr.toUpperCase();

    Serial.println("[LoRa] Sending Packet with Hash: " + sentHashStr);

    // 4. Send AT Command
    // Flush Rx buffer first
    while(Serial1.available()) Serial1.read();
    
    sendAT("AT+TEST=TXLRPKT,\"" + hexPayload + "\"");

    // 5. Wait for TX DONE
    uint32_t tStart = millis();
    bool txDone = false;
    while(millis() - tStart < 3000) {
        String line = Serial1.readStringUntil('\n');
        line.trim();
        if (line.indexOf("TX DONE") != -1) {
            txDone = true;
            break;
        }
    }

    if (!txDone) {
        Serial.println("[LoRa] Error: TX Timeout.");
        return false;
    }

    // 6. Wait for ACK (TCP-like handshake)
    // The receiver should send back the HMAC we just sent.
    Serial.println("[LoRa] TX Done. Waiting for ACK...");
    
    sendAT("AT+TEST=RXLRPKT"); // Switch to RX mode
    
    tStart = millis();
    bool ackReceived = false;
    String ackBuffer = "";

    // Wait up to 5 seconds for ACK
    while(millis() - tStart < 5000) {
        if (Serial1.available()) {
            char c = Serial1.read();
            if (c == '\n') {
                if (ackBuffer.indexOf("+TEST: RX \"") != -1) {
                    // Extract Content
                    int firstQuote = ackBuffer.indexOf('"');
                    int lastQuote = ackBuffer.lastIndexOf('"');
                    if (firstQuote != -1 && lastQuote > firstQuote) {
                        String content = ackBuffer.substring(firstQuote + 1, lastQuote);
                        content.toUpperCase();
                        // Check if the received content contains our Hash
                        if (content.indexOf(sentHashStr) != -1) {
                            ackReceived = true;
                            Serial.println("[LoRa] Valid ACK received !");
                            break;
                        } else {
                             Serial.println("[LoRa] Received packet but hash mismatch (Duplicate or other source).");
                        }
                    }
                }
                ackBuffer = "";
            } else {
                ackBuffer += c;
            }
        }
    }

    return ackReceived;
}

void loraTask(void *pvParameters) {
    delay(1000);
    pinMode(LED_PIN, OUTPUT);
    
    // Boucle d'initialisation bloquante avec retry
    while (true) {
        if (initLoRaModule()) {
            break;
        }
        Serial.println("[LoRaTask] Erreur Init. Nouvelle tentative dans 5s...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    Serial.println("[LoRaTask] Ready.");

    // --- TIME SYNC AT STARTUP ---
    if(requestTimeSync()) {
        setTimeSyncStatus(true);
    }
    // ----------------------------

    unsigned long lastSyncAttempt = 0;

    for (;;) {
        // Periodic Sync Check (Every 60s if not synced)
        if (!isTimeSynced()) {
            if (millis() - lastSyncAttempt > 60000) {
                lastSyncAttempt = millis();
                if(requestTimeSync()) {
                    setTimeSyncStatus(true);
                }
            }
        }

        SensorData data = getSensorData();

        if (data.valid) {
            digitalWrite(LED_PIN, HIGH);
            
            bool success = sendSecurePacket(data.temperature, data.humidity);
            
            digitalWrite(LED_PIN, LOW);

            if (success) {
                Serial.println("[LoRaTask] Transfer Complete.");
                
                // Wait if User is interacting with UI
                while (isUserActive()) {
                    Serial.println("[LoRaTask] User Active - Holding Sleep...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
                
                Serial.println("[LoRaTask] Sleeping...");
                SleepManager::deepSleep(DEEP_SLEEP_INTERVAL_SEC);
            } else {
                Serial.println("[LoRaTask] Transfer Failed (No ACK). Retrying later...");
                
                while (isUserActive()) {
                    Serial.println("[LoRaTask] User Active - Holding Sleep...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }

                // On failure, maybe sleep for a shorter time or retry immediately?
                // For now, let's sleep to save battery, but maybe shorter.
                SleepManager::deepSleep(DEEP_SLEEP_INTERVAL_SEC);
            }
            
            // On Wake Up (Note: deepSleep reboots or resumes depending on implementation, 
            // but in our fake sleep loop it just returns)
            Serial.println("[LoRaTask] Woke Up. Re-init Serial...");
            Serial1.end();
            Serial1.begin(LORA_BAUD_RATE);
            
            // Wait for new sensor data
            Serial.println("[LoRaTask] Waiting for fresh data...");
            delay(2000); 

        } else {
            Serial.println("[LoRaTask] Waiting for valid sensor data...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}