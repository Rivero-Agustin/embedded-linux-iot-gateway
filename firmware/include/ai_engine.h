#pragma once

#include <Arduino.h>

enum AIState {
    AI_STATE_UNKNOWN = 0,
    AI_STATE_NLOS_NOISE,
    AI_STATE_PEDESTRIAN_APPROACH,
    AI_STATE_STATIC_SAFE,
    AI_STATE_VEHICLE_HAZARD
};

struct AIInferenceResult {
    AIState state;
    const char* label;
    float confidence;
    uint32_t inference_time_ms;
};

// Funciones públicas del motor de inferencia
void initAIEngine();
void ai_feed_sensor_sample(float distance, float rx_power, float fp_power);
bool ai_is_buffer_ready();
AIInferenceResult ai_run_inference();
AIInferenceResult ai_get_last_result();

