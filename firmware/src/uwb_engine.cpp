#include "uwb_engine.h"
#include "telemetry_manager.h"
#include "mqtt_manager.h"

#include <SPI.h>
#include "DW1000Ranging.h"
#include "DW1000.h"
#include "esp_log.h"

extern QueueHandle_t bleCommandQueue;

#include "ai_engine.h"

static const char* LOGTAG = "UWB";

float current_distance = 0.0;
float current_rx_power = 0.0;
float current_fp_power = 0.0;
String ultimo_comando_uwb = "";

void uwb_telemetry_task(void *pvParameters) {
    ESP_LOGI(LOGTAG, "Tarea de telemetría UWB iniciada");
    
    while (1) {
        const char* current_tag = "TAG_UWB_01";
        float measured_distance = getCurrentDistance(); // Metros
        AIInferenceResult ai_res = ai_get_last_result();
        
        // Publicar telemetría enriquecida con predicción TinyML
        publish_uwb_telemetry(global_mqtt_client, current_tag, measured_distance, ai_res.label, ai_res.confidence);
        
        // Frecuencia de publicación MQTT: 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// CALLBACKS DEL UWB - se ejecutan automáticamente en segundo plano cuando la radio recibe datos
void newRange() {
    DW1000Device* dev = DW1000Ranging.getDistantDevice();
    if (dev != nullptr) {
        current_distance = dev->getRange();
        current_rx_power = dev->getRXPower();
        current_fp_power = dev->getFPPower();

        // 1. Alimentar el buffer deslizante de la Red Neuronal (TinyML)
        ai_feed_sensor_sample(current_distance, current_rx_power, current_fp_power);

        // 2. Stream de datos por Serial para monitoreo o Data Logging
        Serial.printf("DATA,%lu,%.3f,%.2f,%.2f\n", millis(), current_distance, current_rx_power, current_fp_power);
    }
}
void newDevice(DW1000Device* device) { ESP_LOGI(LOGTAG, "Dispositivo conectado."); }
void inactiveDevice(DW1000Device* device) { ESP_LOGI(LOGTAG, "Dispositivo desconectado."); }

void onCustomDataReceived(const char* data) {
    ultimo_comando_uwb = String(data);
    ESP_LOGI(LOGTAG, "Comando procesado como variable: %s\n", ultimo_comando_uwb.c_str());

    if (ultimo_comando_uwb == "DORMIR") {
        ESP_LOGW(LOGTAG, "Ejecutando lógica interna de DORMIR...");
    }
}

void initUWB(bool isAnchor){
// Inicializar el bus SPI y el módulo DW1000
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);

    // Asignar las funciones callback a los eventos de la radio
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);
    DW1000Ranging.attachCustomDataReceived(onCustomDataReceived);   //PERSONALIZADO

    if (isAnchor) {
        // Iniciar como Ancla (Le asignamos una MAC fija arbitraria)
        DW1000Ranging.startAsAnchor((char*)"82:17:5B:D5:A9:9A:E2:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);
        ESP_LOGI(LOGTAG, "Iniciado como ANCLA (Anchor)");

    } else {
        // Iniciar como Etiqueta (Le asignamos otra MAC fija arbitraria)
        DW1000Ranging.startAsTag((char*)"7D:00:22:EA:82:60:3B:9C", DW1000.MODE_LONGDATA_RANGE_ACCURACY);
        ESP_LOGI(LOGTAG, "Iniciado como ETIQUETA (Tag)");
    }

    // --- CALIBRACIÓN DE ANTENA ---
    DW1000.setAntennaDelay(17000);
}

void processUWB() {
    // 1. Mantiene el motor de distancias girando
    DW1000Ranging.loop();

    // 2. Revisa silenciosamente si el Núcleo 0 (BLE) mandó algo
    char cmdRecibido[50];
    if (xQueueReceive(bleCommandQueue, &cmdRecibido, 0)) {
        ESP_LOGI(LOGTAG, "Inyectando comando al aire: %s\n", cmdRecibido);
        
        // Llamamos a la función "hackeada" que crearemos en el Paso 4
        DW1000Ranging.transmitCustomData(cmdRecibido);
    }
}

float getCurrentDistance() {
    return current_distance;
}

float getCurrentRXPower() {
    return current_rx_power;
}

float getCurrentFPPower() {
    return current_fp_power;
}