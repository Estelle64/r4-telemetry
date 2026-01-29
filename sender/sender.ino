#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "src/config.h"
#include "src/tasks/lora_task.h"
#include "src/tasks/sensor_task.h"
#include "src/utils/data_manager.h"
#include "src/utils/sleep_manager.h"

void setup() {
  Serial.begin(115200);
  // Attente du port série uniquement en debug, sinon ça bloque le sleep sur batterie
  // unsigned long start = millis();
  // while (!Serial && millis() - start < 3000);

  Serial.println("--- Démarrage Sender LoRa ---");

  initDataManager();
  SleepManager::begin();

  BaseType_t res;

  // Sensor Task: Lecture DHT22
  res = xTaskCreate(sensorTask, "SensorTask", 256, NULL, 1, NULL);
  if (res == pdPASS) Serial.println("Tâche Sensor créée.");
  else Serial.println("ERREUR: Echec création tâche Sensor !");

  // LoRa Task: Envoi et Gestion du Sleep
  // Priorité plus élevée (2) pour qu'elle prenne la main dès que possible
  res = xTaskCreate(loraTask, "LoRaTask", 512, NULL, 2, NULL);
  if (res == pdPASS) Serial.println("Tâche LoRa créée.");
  else Serial.println("ERREUR: Echec création tâche LoRa !");

  Serial.println("Démarrage du Scheduler...");
  vTaskStartScheduler();
}

void loop() {
  // Vide (FreeRTOS gère tout)
}
