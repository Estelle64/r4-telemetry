#include "data_manager.h"
#include <Arduino_FreeRTOS.h>

static SensorData currentData;
static bool userActive = false;
static bool dhtConnected = false;
static bool timeSynced = false; // New flag
static SemaphoreHandle_t dataMutex;

void initDataManager() {
    dataMutex = xSemaphoreCreateBinary();
    if (dataMutex != NULL) {
        xSemaphoreGive(dataMutex);
    }
    currentData.valid = false;
    currentData.temperature = 0.0;
    currentData.humidity = 0.0;
    userActive = false;
    dhtConnected = false;
    timeSynced = false;
}
// ...
void setDhtStatus(bool isConnected) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        dhtConnected = isConnected;
        xSemaphoreGive(dataMutex);
    }
}

void setTimeSyncStatus(bool isSynced) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        timeSynced = isSynced;
        xSemaphoreGive(dataMutex);
    }
}

bool isTimeSynced() {
    bool synced = false;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        synced = timeSynced;
        xSemaphoreGive(dataMutex);
    }
    return synced;
}

void updateSensorData(float temp, float hum) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentData.temperature = temp;
        currentData.humidity = hum;
        currentData.valid = true;
        xSemaphoreGive(dataMutex);
    }
}

SensorData getSensorData() {
    SensorData data;
    // Default invalid
    data.valid = false;
    
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        data = currentData;
        xSemaphoreGive(dataMutex);
    }
    return data;
}

void setUserActive(bool isActive) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        userActive = isActive;
        xSemaphoreGive(dataMutex);
    }
}

bool isUserActive() {
    bool active = false;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        active = userActive;
        xSemaphoreGive(dataMutex);
    }
    return active;
}

