#pragma once // Evita inclusiones duplicadas
#include <Arduino.h>

#include "mqtt_client.h"

void publish_uwb_telemetry(esp_mqtt_client_handle_t client, const char* tag_id, float distance_m, const char* ai_state = "unknown", float ai_confidence = 0.0f);