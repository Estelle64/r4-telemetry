#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>

struct SystemData {
    // Valeurs des capteurs
    float localTemperature;
    float localHumidity;
    float remoteTemperature;
    float remoteHumidity;
    uint32_t lastLoRaUpdate;

    // Statut des modules
    bool loraModuleConnected;
    bool dhtModuleConnected;
    bool timeSynced;
};

void initDataManager();

// Fonctions de mise à jour des données
void updateLocalData(float temp, float hum);
void updateRemoteData(float temp, float hum);

// Fonctions de mise à jour des statuts
void setLoraStatus(bool isConnected);
void setDhtStatus(bool isConnected);
void setTimeSyncStatus(bool isSynced);

// Fonction pour récupérer tout l'état
SystemData getSystemData();

#endif // DATA_MANAGER_H