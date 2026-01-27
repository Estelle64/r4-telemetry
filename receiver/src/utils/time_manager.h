#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

/**
 * @brief Syncs the internal RTC with an NTP server.
 * Warning: WiFi must be connected before calling this function.
 * 
 * @return true if sync was successful, false otherwise.
 */
bool syncTimeWithNTP();

#endif
