#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "Estelle"
#define WIFI_PASS "motdepasse"

// LoRa Configuration (Pins pour Arduino R4 WiFi)
#define LORA_SS_PIN 10
#define LORA_RST_PIN 9
#define LORA_DIO0_PIN 2

// Blockchain/Security Configuration
#define LORA_SHARED_SECRET "IoT_Secure_P@ssw0rd_2026"
#define GENESIS_HASH "0000000000000000000000000000000000000000000000000000000000000000"

// Sensor Configuration
#define SENSOR_DHT_PIN 4
#define SENSOR_DHT_TYPE DHT22 // Change to DHT11 if needed

#endif // CONFIG_H