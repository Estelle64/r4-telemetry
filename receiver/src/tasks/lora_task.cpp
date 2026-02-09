#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <RTC.h> // Required for Time Sync
#include "../config.h"
#include "../utils/data_manager.h"
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
            s += (char)Serial1.read();
        }
        delay(1); 
    }
    return s;
}

static bool initLoRaModule() {
    Serial.println("[LoRa] Initialisation du module...");
    Serial1.begin(LORA_BAUD_RATE);
    
    // 1. Test de présence (AT)
    // On essaie plusieurs fois
    for (int i = 0; i < 3; i++) {
        // Vider le buffer
        while(Serial1.available()) Serial1.read();
        
        sendAT("AT");
        String resp = readResponse(1000);
        
        if (resp.indexOf("OK") != -1) {
            Serial.println("[LoRa] Module détecté !");
            
            // 2. Configuration
            sendAT("AT+MODE=TEST"); delay(100); readResponse();
            sendAT("AT+TEST=RFCFG,868,SF7,125,12,15,14"); delay(100); readResponse();
            
            // 3. Passage en mode réception
            sendAT("AT+TEST=RXLRPKT");
            delay(200);
            readResponse(); // Flush confirm

            return true;
        }
        Serial.println("[LoRa] Pas de réponse... tentative " + String(i+1));
        delay(500);
    }
    
    Serial.println("[LoRa] ERREUR CRITIQUE: Module LoRa non détecté.");
    return false;
}

static void sendAck(const uint8_t *hashToSend) {
// ... (reste des fonctions inchangées)
    String hashStr = hashToString(hashToSend);
    Serial.println("[LoRa] Sending ACK: " + hashStr);
    
    // Clear buffer
    while(Serial1.available()) Serial1.read();

    // Send ACK
    sendAT("AT+TEST=TXLRPKT,\"" + hashStr + "\"");
    
    // Wait for TX DONE
    uint32_t tStart = millis();
    while(millis() - tStart < 2000) {
        String line = Serial1.readStringUntil('\n');
        if (line.indexOf("TX DONE") != -1) break;
    }

    // Re-arm RX
    sendAT("AT+TEST=RXLRPKT");
}

// --- GLOBALS ---
static int lastRssi = 0;
static int lastSnr = 0;

static void processRxLine(String line) {
    // 1. Detection RSSI/SNR (Parfois sur une ligne separate ou incluse)
    if (line.indexOf("RSSI:") != -1) {
        int rIdx = line.indexOf("RSSI:");
        int sIdx = line.indexOf("SNR:");
        if (rIdx != -1) lastRssi = line.substring(rIdx + 5).toInt();
        if (sIdx != -1) lastSnr = line.substring(sIdx + 4).toInt();
        
        // Si la ligne ne contient que ca, on s'arrete
        if (line.indexOf("RX \"") == -1) return;
    }

    int rxIndex = line.indexOf("+TEST: RX \"");
    if (rxIndex == -1) return;

    int firstQuote = line.indexOf('"', rxIndex);
    int lastQuote = line.lastIndexOf('"');

    if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
        String hexContent = line.substring(firstQuote + 1, lastQuote);
        
        // Si le RSSI n'a pas été trouvé plus haut (fallback parsing fin de ligne)
        int rssi = lastRssi;
        int snr = lastSnr;

        // Expect: 7 bytes Data + 32 bytes HMAC = 39 bytes => 78 hex chars
        if (hexContent.length() == 78) { 
            uint8_t payload[39];
            hexStringToBytes(hexContent, payload, 39);

            uint8_t dataPart[7];
            uint8_t receivedHash[32];
            
            memcpy(dataPart, payload, 7);
            memcpy(receivedHash, payload + 7, 32);

            // Verify HMAC
            uint8_t calculatedHash[32];
            hmac_sha256(
                (const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET),
                dataPart, 7, 
                calculatedHash
            );

            if (memcmp(receivedHash, calculatedHash, 32) == 0) {
                // Determine TYPE
                uint8_t msgType = dataPart[1];

                if (msgType == 2) { 
                    // --- TYPE 2: DATA REPORT ---
                    uint8_t sensorId = dataPart[0];
                    uint8_t sequence = dataPart[2];
                    float tempVal = (int16_t)(dataPart[3] | (dataPart[4] << 8)) / 100.0;
                    float humVal = (int16_t)(dataPart[5] | (dataPart[6] << 8)) / 100.0;

                    Serial.println("\n========== [LoRa] PAQUET DATA RECU ==========");
                    Serial.print("  Source ID   : "); Serial.print(sensorId);
                    if (sensorId == CAFETERIA_ID)      Serial.println(" (CAFETERIA)");
                    else if (sensorId == FABLAB_ID)    Serial.println(" (FABLAB)");
                    else                               Serial.println(" (INCONNU)");
                    Serial.print("  Sequence    : "); Serial.println(sequence);
                    Serial.print("  Temperature : "); Serial.print(tempVal); Serial.println(" C");
                    Serial.print("  Humidite    : "); Serial.print(humVal); Serial.println(" %");
                    Serial.print("  RSSI / SNR  : "); Serial.print(rssi); Serial.print(" / "); Serial.println(snr);
                    Serial.println("=============================================");

                    updateRemoteData(sensorId, tempVal, humVal, rssi, snr, sequence);
                    setLoraStatus(true);
                    
                    digitalWrite(LED_PIN, HIGH);
                    delay(50);
                    digitalWrite(LED_PIN, LOW);

                    // Send Standard ACK (Hash)
                    sendAck(receivedHash);

                } else if (msgType == 3) {
                    // --- TYPE 3: TIME REQUEST ---
                    Serial.println("\n[LoRa] TIME SYNC REQUEST RECEIVED.");
                    
                    // Get Current Time
                    RTCTime current;
                    RTC.getTime(current);
                    uint32_t now = current.getUnixTime();
                    Serial.print("  Current Unix Time: "); Serial.println(now);

                    // Prepare Response Packet (Type 4)
                    // Format: [ID (Target), TYPE (4), TIME (4 Bytes), HMAC (32 Bytes)]
                    uint8_t respPayload[6];
                    respPayload[0] = dataPart[0]; // Target = Requestor ID
                    respPayload[1] = 4;           // Type 4 = Time Response
                    respPayload[2] = (uint8_t)(now & 0xFF);
                    respPayload[3] = (uint8_t)((now >> 8) & 0xFF);
                    respPayload[4] = (uint8_t)((now >> 16) & 0xFF);
                    respPayload[5] = (uint8_t)((now >> 24) & 0xFF);

                    // Sign
                    uint8_t respHmac[32];
                    hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), respPayload, 6, respHmac);

                    // To Hex
                    String hexResp = "";
                    for(int i=0; i<6; i++) hexResp += byteToHexString(respPayload[i]);
                    for(int i=0; i<32; i++) hexResp += byteToHexString(respHmac[i]);

                    // Send
                    Serial.println("[LoRa] Sending Time Response...");
                    while(Serial1.available()) Serial1.read();
                    sendAT("AT+TEST=TXLRPKT,\"" + hexResp + "\"");
                    
                    // Wait TX DONE
                    uint32_t tStart = millis();
                    while(millis() - tStart < 2000) {
                        String line = Serial1.readStringUntil('\n');
                        if (line.indexOf("TX DONE") != -1) break;
                    }
                    
                    // Re-arm RX
                    sendAT("AT+TEST=RXLRPKT");
                }

            } else {
                Serial.println("[LoRa] ERROR: Invalid HMAC signature.");
            }
        } else {
             Serial.println("[LoRa] Ignored: Invalid length (" + String(hexContent.length()) + ")");
        }
    } else {
        Serial.println("[LoRa] Parse error.");
    }
}

void loraTask(void *pvParameters) {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Boucle d'initialisation (Retry infini tant que pas connecté)
    while (true) {
        if (initLoRaModule()) {
            setLoraStatus(true);
            break; // Sortie de la boucle d'init pour aller vers la boucle RX
        } else {
            setLoraStatus(false);
            Serial.println("[LoRaTask] Nouvelle tentative dans 5s...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }

    Serial.println("[LoRaTask] En attente de paquets...");

    for (;;) {
        if (Serial1.available()) {
            String line = Serial1.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                processRxLine(line);
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}

