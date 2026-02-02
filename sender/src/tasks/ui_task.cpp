#include "ui_task.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "../config.h"
#include "../utils/button_manager.h"
#include "../utils/led_matrix_manager.h"
#include "../utils/data_manager.h"
#include <RTC.h>

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
    ScreenState activeScreen = SHOW_TEMP; 
    unsigned long lastInteraction = 0;
    
    const unsigned long TIMEOUT = MENU_TIMEOUT_MS; 

    for (;;) {
        buttons.update();
        unsigned long now = millis();

        bool anyInput = false;

        if (buttons.isLeftPressed()) {
            Serial.println("[UI] Button LEFT");
            anyInput = true;
            if (currentState == SCREEN_OFF) {
                currentState = activeScreen; 
                setUserActive(true); // Prevent Sleep
            } else {
                if (activeScreen == SHOW_TEMP) activeScreen = SHOW_TIME;
                else if (activeScreen == SHOW_HUM) activeScreen = SHOW_TEMP;
                else if (activeScreen == SHOW_TIME) activeScreen = SHOW_HUM;
                currentState = activeScreen;
            }
        } else if (buttons.isRightPressed()) {
            Serial.println("[UI] Button RIGHT");
            anyInput = true;
            if (currentState == SCREEN_OFF) {
                currentState = activeScreen;
                setUserActive(true); // Prevent Sleep
            } else {
                if (activeScreen == SHOW_TEMP) activeScreen = SHOW_HUM;
                else if (activeScreen == SHOW_HUM) activeScreen = SHOW_TIME;
                else if (activeScreen == SHOW_TIME) activeScreen = SHOW_TEMP;
                currentState = activeScreen;
            }
        }

        if (anyInput) {
            lastInteraction = now;
            setUserActive(true);
        }

        if (currentState != SCREEN_OFF) {
            if (now - lastInteraction > TIMEOUT) {
                Serial.println("[UI] Timeout -> Screen OFF");
                currentState = SCREEN_OFF;
                clearLedMatrix();
                setUserActive(false); // Allow Sleep
            }
        }

        // Render
        if (currentState != SCREEN_OFF) {
            SensorData data = getSensorData(); // Sender specific

            switch (currentState) {
                case SHOW_TEMP:
                     displayTemperatureOnMatrix(data.temperature);
                    break;
                case SHOW_HUM:
                     displayHumidityOnMatrix(data.humidity);
                    break;
                case SHOW_TIME:
                    RTCTime current;
                    RTC.getTime(current);
                    if (current.getYear() < 2024) { 
                         displayTimeOnMatrix(0, 0); 
                    } else {
                        displayTimeOnMatrix(current.getHour(), current.getMinutes());
                    }
                    break;
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}