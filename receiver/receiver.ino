#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "src/config.h"
#include "src/tasks/wifi_task.h"
#include "src/tasks/lora_task.h"
#include "src/tasks/sensor_task.h"
#include "src/utils/data_manager.h"

void setup() {
  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 3000);

  Serial.println("--- Initialisation du Gateway IoT ---");

  initDataManager();

  // Création des tâches FreeRTOS (Tailles optimisées pour le R4 WiFi)
  BaseType_t res;

  // WiFi Task: Gestion du Serveur Web et de la Matrice LED (quand déconnecté)
  res = xTaskCreate(wifiTask, "WiFiTask", 512, NULL, 1, NULL);
  if (res == pdPASS) Serial.println("Tâche WiFi créée.");
  else Serial.println("ERREUR: Echec création tâche WiFi !");

  // LoRa Task: Réception des paquets
  res = xTaskCreate(loraTask, "LoRaTask", 512, NULL, 2, NULL);
  if (res == pdPASS) Serial.println("Tâche LoRa créée.");
  else Serial.println("ERREUR: Echec création tâche LoRa !");

  // Sensor Task: Lecture DHT22 local
  res = xTaskCreate(sensorTask, "SensorTask", 256, NULL, 1, NULL);
  if (res == pdPASS) Serial.println("Tâche Sensor créée.");
  else Serial.println("ERREUR: Echec création tâche Sensor !");

  Serial.println("Démarrage du Scheduler...");
  vTaskStartScheduler();
}

void loop() {
  // Vide
}
