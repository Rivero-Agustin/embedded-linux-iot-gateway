#pragma once // Evita inclusiones duplicadas
#include <Arduino.h>

// Pantalla OLED
#define I2C_SDA 4
#define I2C_SCL 5

// Funciones públicas
void initDisplay();
void updateDisplay(bool isAnchor, float distance);
void showDebugMessage(const char* message);