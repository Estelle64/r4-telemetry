#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>
#include "../config.h"

struct RemoteSensorData {
    float temperature = NAN;
    float humidity = NAN;
    uint32_t lastUpdate = 0;
};

struct SystemData {
    // Valeurs des capteurs
    float localTemperature;
    float localHumidity;
    
    // Données des capteurs distants
    RemoteSensorData cafeteria;
    RemoteSensorData fablab;

    // Statut des modules
    bool loraModuleConnected;
    bool dhtModuleConnected;
    bool timeSynced;
};

void initDataManager();

// Fonctions de mise à jour des données
void updateLocalData(float temp, float hum);
void updateRemoteData(uint8_t sensorId, float temp, float hum);

// Fonctions de mise à jour des statuts
void setLoraStatus(bool isConnected);
void setDhtStatus(bool isConnected);
void setTimeSyncStatus(bool isSynced);

// Fonction pour récupérer tout l'état
SystemData getSystemData();

#endif // DATA_MANAGER_H