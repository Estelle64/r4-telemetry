#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "iPhone de Sean"
#define WIFI_PASS "devinelol"

// LoRa Configuration (Pins pour Arduino R4 WiFi)
#define LORA_SS_PIN 10
#define LORA_RST_PIN 9
#define LORA_DIO0_PIN 2

// Sensor Configuration
#define SENSOR_DHT_PIN 4
#define SENSOR_DHT_TYPE DHT22 // Change to DHT11 if needed

#endif // CONFIG_H