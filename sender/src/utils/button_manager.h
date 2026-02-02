#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

class ButtonManager {
public:
    ButtonManager(uint8_t pinLeft, uint8_t pinRight);
    void begin();
    
    // Call this periodically (e.g. every 10-50ms)
    void update();

    bool isLeftPressed();
    bool isRightPressed();
    bool isAnyPressed();

private:
    uint8_t _pinLeft;
    uint8_t _pinRight;

    bool _lastStateLeft;
    bool _lastStateRight;
    
    bool _pressedLeft;
    bool _pressedRight;

    unsigned long _lastDebounceTimeLeft;
    unsigned long _lastDebounceTimeRight;
    const unsigned long _debounceDelay = 50;
};

#endif
