#pragma once
#include <Arduino.h>

// Chip DW1000
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 21
#define PIN_RST 27
#define PIN_IRQ 34

// Funciones públicas
void uwb_telemetry_task(void *pvParameters);
void initUWB(bool isAnchor);
void processUWB();
float getCurrentDistance();