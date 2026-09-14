#include "ai_engine.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "esp_log.h"
#include <string.h>

static const char* LOGTAG = "EDGE_AI";

// Buffer deslizante para almacenar las últimas 15 muestras (45 floats: distance, rx_power, fp_power)
static float ai_raw_buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };
static size_t samples_collected = 0;
static AIInferenceResult last_result = { AI_STATE_UNKNOWN, "init", 0.0f, 0 };

// Función de callback requerida por el SDK de Edge Impulse para leer el buffer
static int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, ai_raw_buffer + offset, length * sizeof(float));
    return 0;
}

void initAIEngine() {
    memset(ai_raw_buffer, 0, sizeof(ai_raw_buffer));
    samples_collected = 0;
    last_result = { AI_STATE_UNKNOWN, "esperando_datos", 0.0f, 0 };
    ESP_LOGI(LOGTAG, "Motor TinyML Edge Impulse inicializado. Ventana: %d muestras (%d valores)",
             EI_CLASSIFIER_RAW_SAMPLE_COUNT, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
}

void ai_feed_sensor_sample(float distance, float rx_power, float fp_power) {
    // Desplazar el buffer hacia la izquierda 3 posiciones (1 muestra temporal = 3 ejes)
    memmove(ai_raw_buffer, ai_raw_buffer + EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME, 
            (EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) * sizeof(float));

    // Insertar la nueva muestra en la última posición
    size_t last_idx = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE - EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
    ai_raw_buffer[last_idx + 0] = distance;
    ai_raw_buffer[last_idx + 1] = rx_power;
    ai_raw_buffer[last_idx + 2] = fp_power;

    if (samples_collected < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        samples_collected++;
    }
}

bool ai_is_buffer_ready() {
    return samples_collected >= EI_CLASSIFIER_RAW_SAMPLE_COUNT;
}

AIInferenceResult ai_run_inference() {
    if (!ai_is_buffer_ready()) {
        last_result.state = AI_STATE_UNKNOWN;
        last_result.label = "buffer_llenando";
        last_result.confidence = 0.0f;
        return last_result;
    }

    signal_t signal;
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &raw_feature_get_data;

    ei_impulse_result_t result = { 0 };

    uint32_t t_start = millis();
    EI_IMPULSE_ERROR r = run_classifier(&signal, &result, false);
    uint32_t t_elapsed = millis() - t_start;

    if (r != EI_IMPULSE_OK) {
        ESP_LOGE(LOGTAG, "Error ejecutando clasificador Edge Impulse (%d)", r);
        return last_result;
    }

    // Buscar la clase con mayor probabilidad
    float best_confidence = 0.0f;
    const char* best_label = "unknown";
    AIState best_state = AI_STATE_UNKNOWN;

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (result.classification[ix].value > best_confidence) {
            best_confidence = result.classification[ix].value;
            best_label = result.classification[ix].label;
        }
    }

    // Mapear string a Enum
    if (strcmp(best_label, "vehicle_hazard") == 0) {
        best_state = AI_STATE_VEHICLE_HAZARD;
    } else if (strcmp(best_label, "pedestrian_approach") == 0) {
        best_state = AI_STATE_PEDESTRIAN_APPROACH;
    } else if (strcmp(best_label, "static_safe") == 0) {
        best_state = AI_STATE_STATIC_SAFE;
    } else if (strcmp(best_label, "nlos_noise") == 0) {
        best_state = AI_STATE_NLOS_NOISE;
    }

    last_result.state = best_state;
    last_result.label = best_label;
    last_result.confidence = best_confidence;
    last_result.inference_time_ms = t_elapsed;

    ESP_LOGD(LOGTAG, "Prediccion: %s (%.1f%%) en %lu ms", best_label, best_confidence * 100.0f, t_elapsed);

    return last_result;
}

AIInferenceResult ai_get_last_result() {
    return last_result;
}

