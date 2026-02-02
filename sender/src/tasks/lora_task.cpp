#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <RTC.h>
#include "../config.h"
#include "../utils/data_manager.h"
#include "../utils/sleep_manager.h"

// --- CONFIGURATION ---
const int LED_PIN = 13;
const int LORA_BAUD_RATE = 9600;

// --- SHA256 / HMAC IMPLEMENTATION (Minimal) ---
struct SHA256_CTX {
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
};

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))

static const uint32_t k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d; d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0; ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85; ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c; ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen++] = data[i];
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += ctx->datalen * 8;
    for(int k=0; k<8; k++) ctx->data[63-k] = (uint8_t)(ctx->bitlen >> (k*8)); // Big Endian
    sha256_transform(ctx, ctx->data);
    for (i = 0; i < 4; ++i) {
        hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0xff;
        hash[i+4] = (ctx->state[1] >> (24 - i * 8)) & 0xff;
        hash[i+8] = (ctx->state[2] >> (24 - i * 8)) & 0xff;
        hash[i+12] = (ctx->state[3] >> (24 - i * 8)) & 0xff;
        hash[i+16] = (ctx->state[4] >> (24 - i * 8)) & 0xff;
        hash[i+20] = (ctx->state[5] >> (24 - i * 8)) & 0xff;
        hash[i+24] = (ctx->state[6] >> (24 - i * 8)) & 0xff;
        hash[i+28] = (ctx->state[7] >> (24 - i * 8)) & 0xff;
    }
}

static void hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output) {
    uint8_t k_ipad[64], k_opad[64], tk[32];
    if (key_len > 64) {
        SHA256_CTX ctx; sha256_init(&ctx); sha256_update(&ctx, key, key_len); sha256_final(&ctx, tk);
        key = tk; key_len = 32;
    }
    memset(k_ipad, 0x36, 64); memset(k_opad, 0x5c, 64);
    for (size_t i = 0; i < key_len; i++) { k_ipad[i] ^= key[i]; k_opad[i] ^= key[i]; }
    
    SHA256_CTX ctx;
    uint8_t innerHash[32];
    sha256_init(&ctx); sha256_update(&ctx, k_ipad, 64); sha256_update(&ctx, data, data_len); sha256_final(&ctx, innerHash);
    sha256_init(&ctx); sha256_update(&ctx, k_opad, 64); sha256_update(&ctx, innerHash, 32); sha256_final(&ctx, output);
}

// --- UTILS ---

static String byteToHexString(uint8_t b) {
    String s = String(b, HEX);
    if (s.length() < 2) s = "0" + s;
    return s;
}

static String hashToString(const uint8_t *hash) {
    String s = "";
    for(int i = 0; i < 32; i++) s += byteToHexString(hash[i]);
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

// --- LORA LOGIC ---

// Request Time from Receiver
// Packet: [ID, TYPE=3, 0,0,0,0 (Padding), HMAC]
static void requestTimeSync() {
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
    if(!txDone) { Serial.println("[LoRa] TX Error during sync."); return; }

    // 6. Wait for Response (Type 4)
    Serial.println("[LoRa] Waiting for Time Response...");
    sendAT("AT+TEST=RXLRPKT");
    
    tStart = millis();
    // Wait longer for sync (e.g. 5s)
    while(millis() - tStart < 5000) {
        if (Serial1.available()) {
            char c = Serial1.read();
            if (c == '\n') {
                String line = Serial1.readStringUntil('\n'); // Should catch full line? 
                // Wait, process char by char buffer better
            }
        }
        // Simplified parsing loop reused from sendSecurePacket but adapted
        // Actually let's just do a simple buffer read like before
    }
    
    // RE-IMPLEMENTING SIMPLE READ LOOP FOR SYNC
    String rxBuffer = "";
    tStart = millis();
    while(millis() - tStart < 5000) {
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
                            // Decode
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
                                return; 
                            }
                        }
                    }
                }
                rxBuffer = "";
            }
        }
    }
    Serial.println("[LoRa] Time Sync Failed (Timeout).");
}

// Send Packet: [ID, Type, T_low, T_high, H_low, H_high] + [HMAC(32)]
static bool sendSecurePacket(float temp, float hum) {
    // 1. Prepare Data
    uint8_t payload[6]; 
    int16_t tInt = (int16_t)(temp * 100);
    int16_t hInt = (int16_t)(hum * 100);

    payload[0] = SENSOR_ID;
    payload[1] = 2; // Type 2 = Mixed
    payload[2] = (uint8_t)(tInt & 0xFF);
    payload[3] = (uint8_t)((tInt >> 8) & 0xFF);
    payload[4] = (uint8_t)(hInt & 0xFF);
    payload[5] = (uint8_t)((hInt >> 8) & 0xFF);

    // 2. Sign Data (HMAC)
    uint8_t hmac[32];
    hmac_sha256((const uint8_t*)LORA_SHARED_SECRET, strlen(LORA_SHARED_SECRET), payload, 6, hmac);

    // 3. Build Hex String
    String hexPayload = "";
    for(int i=0; i<6; i++) hexPayload += byteToHexString(payload[i]);
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
    Serial1.begin(LORA_BAUD_RATE);

    Serial.println("[LoRaTask] Configuring Module...");
    sendAT("AT+MODE=TEST"); delay(100); readResponse();
    sendAT("AT+TEST=RFCFG,868,SF7,125,12,15,14"); delay(100); readResponse();
    Serial.println("[LoRaTask] Ready.");

    // --- TIME SYNC AT STARTUP ---
    requestTimeSync();
    // ----------------------------

    for (;;) {
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