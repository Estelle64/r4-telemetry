#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../utils/data_manager.h"

// --- CONSTANTES & DEFINITIONS ---
const int LED_PIN = 13;
static String buffer = ""; // Buffer persistant pour la tâche

// Structure du paquet LoRa (identique à l émetteur)
typedef struct {
    uint8_t sensor_id;
    uint8_t data_type; // 0=Temp, 1=Hum
    int16_t value;     // Valeur * 100
} sensor_data_t;

// --- FONCTIONS UTILITAIRES (Helper functions) ---

// Calcul CRC-8 simple (polynôme 0x07)
static uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0) {
                crc = (uint8_t)((crc << 1) ^ 0x07);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// Convertit une chaîne ASCII hexadécimale (ex: "3031") en chaîne ASCII (ex: "01")
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

// Convertit une chaîne ASCII hexadécimale en bytes bruts
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
        delay(1); // Petit yield pour éviter de bloquer totalement
    }
    return s;
}

// Traitement du buffer complet
static void processBuffer() {
    // Debug sur Serial USB
    // Serial.println("LoRa Raw: " + buffer);

    // Chercher le message hexadécimal reçu via AT+TEST=RXLRPKT
    // Format typique: +TEST: RX "..."
    if (buffer.indexOf("+TEST: RX") != -1) {
        int firstQuote = buffer.indexOf('"');
        int lastQuote = buffer.lastIndexOf('"');

        if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
            String doubleHexContent = buffer.substring(firstQuote + 1, lastQuote);

            // 1. Décodage double-hex
            String hexContent = doubleHexToHexString(doubleHexContent);

            // 2. Vérification taille (5 octets = 10 chars hex)
            if (hexContent.length() == 10) {
                uint8_t packet[5];
                hexStringToBytes(hexContent, packet, 5);

                // 3. Vérification CRC
                uint8_t calculated_crc = crc8(packet, 4);
                if (calculated_crc == packet[4]) {
                    sensor_data_t data;
                    // Attention à l éalignement mémoire, ici copie brute ok car struct packed ou simple
                    // Copie des 4 premiers octets dans la struct (id, type, value)
                    memcpy(&data, packet, 4);

                    float finalValue = data.value / 100.0;

                    // Debug
                    Serial.print("[LoRa] Reçu ID:");
                    Serial.print(data.sensor_id);
                    Serial.print(" Type:");
                    Serial.print(data.data_type);
                    Serial.print(" Val:");
                    Serial.println(finalValue);

                    // Mise à jour du DataManager
                    // On récupère l état actuel pour ne modifier que la valeur reçue
                    SystemData current = getSystemData();
                    float newTemp = current.remoteTemperature;
                    float newHum = current.remoteHumidity;

                    if (data.data_type == 0) { // Température
                        newTemp = finalValue;
                    } else if (data.data_type == 1) { // Humidité
                        newHum = finalValue;
                    }

                    updateRemoteData(newTemp, newHum);

                    // Feedback visuel
                    digitalWrite(LED_PIN, HIGH);
                    delay(50); // Petit délai bloquant acceptable ici (50ms)
                    digitalWrite(LED_PIN, LOW);

                } else {
                    Serial.println("[LoRa] Erreur CRC");
                }
            }
        }
    }
    
    // Nettoyage buffer fait par l appelant
}

// --- TÂCHE PRINCIPALE ---

void loraTask(void *pvParameters) {
    // Petit délai pour laisser le système démarrer proprement
    delay(1000);

    // 1. Initialisation Hardware
    pinMode(LED_PIN, OUTPUT);
    
    // Serial1 est généralement sur les pins 0(RX) et 1(TX) de l Arduino R4 WiFi
    Serial1.begin(9600); 
    
    Serial.println("[LoRaTask] Configuration du module...");

    // Séquence d init AT
    sendAT("AT+MODE=TEST");
    delay(200);
    String resp = readResponse();
    // Si on reçoit OK, le module est présent
    if (resp.indexOf("OK") != -1 || resp.indexOf("TEST") != -1) {
        setLoraStatus(true);
        Serial.println("[LoRaTask] Module détecté OK.");
    } else {
        setLoraStatus(false);
        Serial.println("[LoRaTask] Module non détecté !");
    }
    
    while(Serial1.available()) Serial1.read(); // Flush

    // Config: 868MHz, SF7, BW125, CR4/5, Preamb 8, Power 14dBm
    sendAT("AT+TEST=RFCFG,868,SF7,125,12,15,14");
    delay(200);
    while(Serial1.available()) Serial1.read();

    // Passage en mode réception
    sendAT("AT+TEST=RXLRPKT");
    delay(200);
    Serial.println("[LoRaTask] Réponse Init: " + readResponse());
    Serial.println("[LoRaTask] Prêt.");

    // 2. Boucle Infinie FreeRTOS
    for (;;) {
        // Lecture non bloquante du port série
        while (Serial1.available()) {
            char c = Serial1.read();
            buffer += c;

            if (c == '\n') {
                processBuffer();
                buffer = ""; // Reset du buffer
            }
        }

        // On laisse la main aux autres tâches si rien n arrive
        // delay() appelle vTaskDelay() sous le capot avec Arduino_FreeRTOS
        delay(10); 
    }
}
