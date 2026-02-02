#include "button_manager.h"

ButtonManager::ButtonManager(uint8_t pinLeft, uint8_t pinRight) {
    _pinLeft = pinLeft;
    _pinRight = pinRight;
    _lastStateLeft = HIGH;
    _lastStateRight = HIGH;
    _pressedLeft = false;
    _pressedRight = false;
    _lastDebounceTimeLeft = 0;
    _lastDebounceTimeRight = 0;
}

void ButtonManager::begin() {
    pinMode(_pinLeft, INPUT_PULLUP); // Grove buttons often act as switches, usually Active HIGH or LOW depending on module. 
                                     // Standard Grove Button is Active HIGH (VCC when pressed).
                                     // However, without a physical pull-down on the module, we might need configuration.
                                     // Standard Grove Button Module (v1.2) has a hardware pull-down resistor.
                                     // So INPUT is fine. If it's a raw switch, INPUT_PULLUP is safer.
                                     // Let's assume Active HIGH for Grove Button (pressed = HIGH).
    pinMode(_pinLeft, INPUT);
    pinMode(_pinRight, INPUT);
}

void ButtonManager::update() {
    // Reset "pressed" state for this frame (simulating a 'trigger' event)
    _pressedLeft = false;
    _pressedRight = false;

    // --- LEFT BUTTON ---
    int readingLeft = digitalRead(_pinLeft);
    // Debounce logic could be simpler for a menu, but let's stick to standard change detection
    if (readingLeft != _lastStateLeft) {
        _lastDebounceTimeLeft = millis();
    }

    if ((millis() - _lastDebounceTimeLeft) > _debounceDelay) {
        // If state changed
        static int currentStableLeft = LOW;
        if (readingLeft != currentStableLeft) {
            currentStableLeft = readingLeft;
            if (currentStableLeft == HIGH) {
                _pressedLeft = true;
            }
        }
    }
    _lastStateLeft = readingLeft;

    // --- RIGHT BUTTON ---
    int readingRight = digitalRead(_pinRight);
    if (readingRight != _lastStateRight) {
        _lastDebounceTimeRight = millis();
    }

    if ((millis() - _lastDebounceTimeRight) > _debounceDelay) {
        static int currentStableRight = LOW;
        if (readingRight != currentStableRight) {
            currentStableRight = readingRight;
            if (currentStableRight == HIGH) {
                _pressedRight = true;
            }
        }
    }
    _lastStateRight = readingRight;
}

bool ButtonManager::isLeftPressed() {
    return _pressedLeft;
}

bool ButtonManager::isRightPressed() {
    return _pressedRight;
}

bool ButtonManager::isAnyPressed() {
    return _pressedLeft || _pressedRight;
}
