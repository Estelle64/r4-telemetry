#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <Arduino.h>

class CryptoManager {
public:
    static void hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output);
    static String hashToString(const uint8_t *hash, size_t length);
    static void stringToHash(String hexString, uint8_t *output, size_t length);
};

#endif
