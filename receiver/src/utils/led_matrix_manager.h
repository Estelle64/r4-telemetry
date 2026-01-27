#ifndef LED_MATRIX_MANAGER_H
#define LED_MATRIX_MANAGER_H

#include <Arduino.h>

void initLedMatrix();
void displayTemperatureOnMatrix(int temperature);
void clearLedMatrix();

#endif