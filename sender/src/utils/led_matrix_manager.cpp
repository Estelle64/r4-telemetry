#include "led_matrix_manager.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

// Police 3x5 simplifiée
// 0-9
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

// Lettres T et H (3x5)
const uint8_t CHAR_T[3] = {0x01, 0x1F, 0x01}; // T
const uint8_t CHAR_H[3] = {0x1F, 0x04, 0x1F}; // H

void initLedMatrix() {
    matrix.begin();
}

// Helper pour inverser l'image (Standard)
void render(uint8_t frame[8][12]) {
    uint8_t flipped[8][12];
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 12; c++) {
            // Pas d'inversion
            flipped[r][c] = frame[r][c];
        }
    }
    matrix.renderBitmap(flipped, 8, 12);
}

void clearLedMatrix() {
    uint8_t frame[8][12];
    memset(frame, 0, sizeof(frame));
    render(frame);
}

void drawDigit(uint8_t frame[8][12], int number, int colOffset) {
    if (number < 0 || number > 10) return;
    for (int col = 0; col < 3; col++) {
        uint8_t colData = DIGITS[number][col];
        for (int row = 0; row < 5; row++) {
            // Lecture LSB vers MSB (Bit 0 = Haut, Bit 4 = Bas)
            if ((colData >> row) & 0x01) { 
                if (colOffset + col < 12 && colOffset + col >= 0) {
                   frame[row + 2][colOffset + col] = 1;
                }
            }
        }
    }
}

// Draw small char at top right (cols 9, 10, 11)
void drawSmallChar(uint8_t frame[8][12], const uint8_t charData[3]) {
    for (int col = 0; col < 3; col++) {
        uint8_t colVal = charData[col];
        for (int row = 0; row < 5; row++) {
             // Lecture LSB vers MSB
             if ((colVal >> row) & 0x01) {
                 frame[row + 1][9 + col] = 1; 
             }
        }
    }
}

void displayTemperatureOnMatrix(float tempVal) {
    uint8_t frame[8][12];
    memset(frame, 0, sizeof(frame));
    
    int temperature = (int)tempVal;

    if (temperature < 0 || temperature > 99) {
        // "--"
        frame[3][1] = 1; frame[3][2] = 1;
        frame[3][5] = 1; frame[3][6] = 1;
    } else {
        int tens = temperature / 10;
        int units = temperature % 10;
        
        // Digits at 1 and 5
        if (tens > 0) drawDigit(frame, tens, 1);
        drawDigit(frame, units, 5);
    }

    drawSmallChar(frame, CHAR_T);
    render(frame);
}

void displayHumidityOnMatrix(float humVal) {
    uint8_t frame[8][12];
    memset(frame, 0, sizeof(frame));

    int humidity = (int)humVal;
    if (humidity < 0 || humidity > 99) {
        frame[3][1] = 1; frame[3][2] = 1; 
        frame[3][5] = 1; frame[3][6] = 1;
    } else {
        int tens = humidity / 10;
        int units = humidity % 10;
        if (tens > 0) drawDigit(frame, tens, 1);
        drawDigit(frame, units, 5);
    }

    drawSmallChar(frame, CHAR_H);
    render(frame);
}

void displayTimeOnMatrix(int hours, int minutes) {
    // Scrolling text for HH:MM
    // To simplify: display HH then MM or scroll. 
    // User asked for "Juste heure minute".
    // 12x8 is small. Let's try to fit 4 digits densely? 
    // 3px * 4 = 12px. No gaps. 
    // 12:34 -> [1][2][3][4]
    // 1: 000, 2: 000, ...
    // It's tight but readable if we use the full width.
    // Let's try static first: HHMM.
    
    uint8_t frame[8][12];
    memset(frame, 0, sizeof(frame));
    
    int h1 = hours / 10;
    int h2 = hours % 10;
    int m1 = minutes / 10;
    int m2 = minutes % 10;
    
    // Draw packed
    // x=0, x=3, x=6, x=9 (each 3 wide). Perfect 12 pixels.
    drawDigit(frame, h1, 0);
    drawDigit(frame, h2, 3);
    
    // Blink dots for separator?
    // frame[3][6] = 1; // Center? No space.
    // Just display numbers.
    
    drawDigit(frame, m1, 6);
    drawDigit(frame, m2, 9);
    
    // Add a dot at bottom center to separate? 
    // Maybe blink a pixel at (7, 6)
    static bool blink = false;
    blink = !blink;
    if(blink) frame[7][6] = 1;

    render(frame);
}
