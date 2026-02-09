#ifndef SECURITY_UTILS_H
#define SECURITY_UTILS_H

#include <Arduino.h>

void hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output);
String hashToString(const uint8_t *hash);

#endif