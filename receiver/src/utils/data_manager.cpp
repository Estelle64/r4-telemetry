#include "data_manager.h"
#include <Arduino_FreeRTOS.h>
#include "../config.h"

static SystemData currentState;
static SemaphoreHandle_t dataMutex;

void initDataManager() {
    dataMutex = xSemaphoreCreateBinary();
    if (dataMutex != NULL) {
        xSemaphoreGive(dataMutex);
    }
    memset(&currentState, 0, sizeof(SystemData));
    currentState.loraModuleConnected = false;
    currentState.dhtModuleConnected = false;
    currentState.wifiConnected = false;
    currentState.mqttConnected = false;
    currentState.timeSynced = false;
}

void updateLocalData(float temp, float hum) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.localTemperature = temp;
        currentState.localHumidity = hum;
        xSemaphoreGive(dataMutex);
    }
}

static void updateRemoteSensor(RemoteSensorData &sensor, float temp, float hum, int rssi, int snr, uint8_t sequence) {
    sensor.temperature = temp;
    sensor.humidity = hum;
    sensor.lastUpdate = millis();
    
    // N'utiliser le RSSI/SNR que s'ils sont fournis (non nuls)
    if (rssi != 0) sensor.rssi = rssi;
    if (snr != 0) sensor.snr = snr;
    
    // Detection de perte de paquets
    if (sensor.packetsReceived > 0) {
        if (sequence == sensor.lastSequence) {
            // Doublon : on incrémente les reçus mais on ne touche pas aux pertes
            sensor.packetsReceived++;
            return; 
        }

        int expected = (sensor.lastSequence + 1) & 0xFF;
        if (sequence != expected) {
            // Calcul de l'écart
            int gap = (sequence > expected) ? (sequence - expected) : (256 - expected + sequence);
            
            // Si le gap est énorme (ex > 100), c'est probablement un RESET du sender
            // On ne le compte pas comme une perte massive
            if (gap < 100) {
                sensor.packetsLost += gap;
            } else {
                Serial.println("[DataManager] Reset du Sender detecte (Saut de sequence ignore)");
            }
        }
    }
    sensor.lastSequence = sequence;
    sensor.packetsReceived++;
}

void updateRemoteData(uint8_t sensorId, float temp, float hum, int rssi, int snr, uint8_t sequence) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        if (sensorId == CAFETERIA_ID) {
            Serial.println("[DataManager] Mise a jour CAFET -> Memoire Partagee");
            updateRemoteSensor(currentState.cafeteria, temp, hum, rssi, snr, sequence);
        } else if (sensorId == FABLAB_ID) {
            Serial.println("[DataManager] Mise a jour FABLAB -> Memoire Partagee");
            updateRemoteSensor(currentState.fablab, temp, hum, rssi, snr, sequence);
        } else {
            Serial.print("[DataManager] ID Inconnu ignore: ");
            Serial.println(sensorId);
        }
        xSemaphoreGive(dataMutex);
    }
}

void setLoraStatus(bool isConnected) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.loraModuleConnected = isConnected;
        xSemaphoreGive(dataMutex);
    }
}

void setDhtStatus(bool isConnected) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.dhtModuleConnected = isConnected;
        xSemaphoreGive(dataMutex);
    }
}

void setWifiStatus(bool isConnected) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.wifiConnected = isConnected;
        xSemaphoreGive(dataMutex);
    }
}

void setMqttStatus(bool isConnected) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.mqttConnected = isConnected;
        xSemaphoreGive(dataMutex);
    }
}

void setTimeSyncStatus(bool isSynced) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.timeSynced = isSynced;
        xSemaphoreGive(dataMutex);
    }
}

void setMqttSequence(const char* id, uint32_t seq) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        if (strcmp(id, "cafeteria") == 0) {
            currentState.mqttSequenceCafet = seq;
            currentState.mqttHandshakeDoneCafet = true;
        } else if (strcmp(id, "fablab") == 0) {
            currentState.mqttSequenceFablab = seq;
            currentState.mqttHandshakeDoneFablab = true;
        }
        xSemaphoreGive(dataMutex);
    }
}

uint32_t getNextMqttSequence(const char* id) {
    uint32_t seq = 0;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        if (strcmp(id, "cafeteria") == 0) seq = currentState.mqttSequenceCafet++;
        else if (strcmp(id, "fablab") == 0) seq = currentState.mqttSequenceFablab++;
        xSemaphoreGive(dataMutex);
    }
    return seq;
}

bool isMqttHandshakeDone(const char* id) {
    bool done = false;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        if (strcmp(id, "cafeteria") == 0) done = currentState.mqttHandshakeDoneCafet;
        else if (strcmp(id, "fablab") == 0) done = currentState.mqttHandshakeDoneFablab;
        xSemaphoreGive(dataMutex);
    }
    return done;
}

void setMqttHandshakeDone(const char* id, bool done) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        if (strcmp(id, "cafeteria") == 0) currentState.mqttHandshakeDoneCafet = done;
        else if (strcmp(id, "fablab") == 0) currentState.mqttHandshakeDoneFablab = done;
        xSemaphoreGive(dataMutex);
    }
}

SystemData getSystemData() {
    SystemData data;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        data = currentState;
        xSemaphoreGive(dataMutex);
    }
    return data;
}