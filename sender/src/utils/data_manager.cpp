#include "data_manager.h"
#include <Arduino_FreeRTOS.h>

static SensorData currentData;
static SemaphoreHandle_t dataMutex;

void initDataManager() {
    dataMutex = xSemaphoreCreateBinary();
    if (dataMutex != NULL) {
        xSemaphoreGive(dataMutex);
    }
    currentData.valid = false;
    currentData.temperature = 0.0;
    currentData.humidity = 0.0;
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
