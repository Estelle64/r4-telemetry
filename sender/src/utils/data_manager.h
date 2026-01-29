#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>

struct SensorData {
    float temperature;
    float humidity;
    bool valid; // Indicates if data is fresh/valid
};

void initDataManager();

// Mettre à jour les données du capteur
void updateSensorData(float temp, float hum);

// Récupérer les données
SensorData getSensorData();

#endif // DATA_MANAGER_H
