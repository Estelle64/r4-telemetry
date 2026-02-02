#ifndef LED_MATRIX_MANAGER_H
#define LED_MATRIX_MANAGER_H

#include <Arduino.h>

void initLedMatrix();
void displayTemperatureOnMatrix(float temperature);
void displayHumidityOnMatrix(float humidity);
void displayTimeOnMatrix(int hours, int minutes);
void clearLedMatrix();

#endif