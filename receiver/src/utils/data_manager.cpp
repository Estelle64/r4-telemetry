#include "data_manager.h"
#include <Arduino_FreeRTOS.h>

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
}

void updateLocalData(float temp, float hum) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.localTemperature = temp;
        currentState.localHumidity = hum;
        xSemaphoreGive(dataMutex);
    }
}

void updateRemoteData(float temp, float hum) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        currentState.remoteTemperature = temp;
        currentState.remoteHumidity = hum;
        currentState.lastLoRaUpdate = millis();
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

SystemData getSystemData() {
    SystemData data;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        data = currentState;
        xSemaphoreGive(dataMutex);
    }
    return data;
}