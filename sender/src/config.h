#ifndef CONFIG_H
#define CONFIG_H

// LoRa Configuration (Pins pour Arduino R4 WiFi)
#define LORA_SS_PIN 10
#define LORA_RST_PIN 9
// Move DIO0 to avoid conflict with buttons (D2/D3)
#define LORA_DIO0_PIN 5 

// UI Configuration
#define BUTTON_LEFT_PIN 2
#define BUTTON_RIGHT_PIN 3
#define MENU_TIMEOUT_MS 10000

// LoRa Protocol
#define SENSOR_ID 0x01 // Identifiant unique de ce capteur

// Blockchain/Security Configuration
#define LORA_SHARED_SECRET "IoT_Secure_P@ssw0rd_2026"
#define GENESIS_HASH "0000000000000000000000000000000000000000000000000000000000000000"

// Sensor Configuration
#define SENSOR_DHT_PIN 4
#define SENSOR_DHT_TYPE DHT22

// Power Management
#define DEEP_SLEEP_INTERVAL_SEC 15 // 15 secondes pour debug

#endif // CONFIG_H