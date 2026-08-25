#pragma once // Evita inclusiones duplicadas
#include <Arduino.h>

#include "mqtt_client.h"

extern esp_mqtt_client_handle_t global_mqtt_client;

void mqtt_app_start(void);