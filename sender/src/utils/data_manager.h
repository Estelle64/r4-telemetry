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

// UI State
void setUserActive(bool isActive);
bool isUserActive();

// Status
void setDhtStatus(bool isConnected);
void setTimeSyncStatus(bool isSynced);
bool isTimeSynced();

#endif // DATA_MANAGER_H
