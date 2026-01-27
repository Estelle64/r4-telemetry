#include "led_matrix_manager.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// Police 3x5 simplifiée pour les chiffres 0-9 et quelques caractères
// Chaque octet représente une colonne (3 colonnes par chiffre)
// Bit 0 = haut, Bit 4 = bas
const uint8_t DIGITS[11][3] = {
  {0x1F, 0x11, 0x1F}, // 0
  {0x00, 0x1F, 0x00}, // 1
  {0x1D, 0x15, 0x17}, // 2
  {0x15, 0x15, 0x1F}, // 3
  {0x07, 0x04, 0x1F}, // 4
  {0x17, 0x15, 0x1D}, // 5
  {0x1F, 0x15, 0x1D}, // 6
  {0x01, 0x01, 0x1F}, // 7
  {0x1F, 0x15, 0x1F}, // 8
  {0x17, 0x15, 0x1F}, // 9
  {0x00, 0x00, 0x00}  // Espace (10)
};

void initLedMatrix() {
    matrix.begin();
}

void clearLedMatrix() {
    uint8_t frame[8][12];
    memset(frame, 0, sizeof(frame));
    matrix.renderBitmap(frame, 8, 12);
}

// Fonction utilitaire pour dessiner un chiffre à une position donnée
void drawDigit(uint8_t frame[8][12], int number, int colOffset) {
    if (number < 0 || number > 10) return;
    
    for (int col = 0; col < 3; col++) {
        uint8_t colData = DIGITS[number][col];
        for (int row = 0; row < 5; row++) {
            // On centre verticalement (row + 1)
            if ((colData >> row) & 0x01) {
                if (colOffset + col < 12) {
                   frame[row + 2][colOffset + col] = 1;
                }
            }
        }
    }
}

void displayTemperatureOnMatrix(int temperature) {
    uint8_t frame[8][12];
    memset(frame, 0, sizeof(frame)); // Clear buffer

    // Gestion des limites d'affichage (0 à 99)
    // Si hors limites, on affiche "--" (symbolisé par des points ici)
    if (temperature < 0 || temperature > 99) {
        frame[3][5] = 1; frame[3][6] = 1;
    } else {
        int tens = temperature / 10;
        int units = temperature % 10;

        // Si < 10, on n'affiche pas le 0 des dizaines
        if (tens > 0) {
            drawDigit(frame, tens, 2); // Position X = 2
        }
        drawDigit(frame, units, 6); // Position X = 6
    }
    
    matrix.renderBitmap(frame, 8, 12);
}