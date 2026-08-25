#pragma once
#include <Arduino.h>

// Funciones públicas para el main
void initWiFi(const char* ssid, const char* password);
bool isWiFiConnected();
String getWiFiIP();
void reconnectWiFi(const char* ssid, const char* password);