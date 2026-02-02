#include "ui_task.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../utils/button_manager.h"
#include "../utils/led_matrix_manager.h"
#include "../utils/data_manager.h"
#include <RTC.h> // For time

// Initialize Button Manager with Config Pins
static ButtonManager buttons(BUTTON_LEFT_PIN, BUTTON_RIGHT_PIN);

enum ScreenState {
    SCREEN_OFF,
    SHOW_TEMP,
    SHOW_HUM,
    SHOW_TIME
};

void uiTask(void *pvParameters) {
    Serial.println("[UITask] Initialisation...");
    
    buttons.begin();
    initLedMatrix();

    ScreenState currentState = SCREEN_OFF;
    ScreenState activeScreen = SHOW_TEMP; // Default start screen when woken
    unsigned long lastInteraction = 0;
    
    const unsigned long TIMEOUT = MENU_TIMEOUT_MS; 

    for (;;) {
        buttons.update();
        unsigned long now = millis();

        bool anyInput = false;

        // --- INPUT HANDLING ---
        if (buttons.isLeftPressed()) {
            Serial.println("[UI] Button LEFT");
            anyInput = true;
            if (currentState == SCREEN_OFF) {
                currentState = activeScreen; // Wake up
            } else {
                // Previous Screen
                if (activeScreen == SHOW_TEMP) activeScreen = SHOW_TIME;
                else if (activeScreen == SHOW_HUM) activeScreen = SHOW_TEMP;
                else if (activeScreen == SHOW_TIME) activeScreen = SHOW_HUM;
                currentState = activeScreen;
            }
        } else if (buttons.isRightPressed()) {
            Serial.println("[UI] Button RIGHT");
            anyInput = true;
            if (currentState == SCREEN_OFF) {
                currentState = activeScreen; // Wake up
            } else {
                // Next Screen
                if (activeScreen == SHOW_TEMP) activeScreen = SHOW_HUM;
                else if (activeScreen == SHOW_HUM) activeScreen = SHOW_TIME;
                else if (activeScreen == SHOW_TIME) activeScreen = SHOW_TEMP;
                currentState = activeScreen;
            }
        }

        if (anyInput) {
            lastInteraction = now;
        }

        // --- TIMEOUT MANAGEMENT ---
        if (currentState != SCREEN_OFF) {
            if (now - lastInteraction > TIMEOUT) {
                Serial.println("[UI] Timeout -> Screen OFF");
                currentState = SCREEN_OFF;
                clearLedMatrix();
            }
        }

        // --- RENDER ---
        if (currentState != SCREEN_OFF) {
            SystemData data = getSystemData();

            switch (currentState) {
                case SHOW_TEMP:
                    if (!isnan(data.localTemperature))
                        displayTemperatureOnMatrix(data.localTemperature);
                    break;
                case SHOW_HUM:
                    if (!isnan(data.localHumidity))
                        displayHumidityOnMatrix(data.localHumidity);
                    break;
                case SHOW_TIME:
                    RTCTime current;
                    RTC.getTime(current);
                    if (current.getYear() < 2024) { // Assumption: If not synced/set, year is default 2000 or near 0
                         // "NULL" representation? 
                         // displayTime handles numbers.
                         // Let's display 00:00 or maybe -- --
                         displayTimeOnMatrix(0, 0); 
                    } else {
                        displayTimeOnMatrix(current.getHour(), current.getMinutes());
                    }
                    break;
            }
        }

        // UI Poll Rate (responsive enough for buttons)
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
