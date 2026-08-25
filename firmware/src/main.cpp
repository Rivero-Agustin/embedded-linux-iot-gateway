#include <Arduino.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "display_manager.h"
#include "uwb_engine.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "nvs_manager.h"
#include "config.h"

static const char* LOGTAG = "MAIN";

QueueHandle_t bleCommandQueue;

// Pantalla y UWB
void taskUWB(void *pvParameters) {
    init_nvs();
    initDisplay();
    initUWB(IS_ANCHOR);

    uint32_t ultimo_refresco = millis();
    int contador_watchdog = 0;

    for(;;) {
        // El motor UWB debe correr sin interrupciones severas
        processUWB();
        // Actualizar la pantalla cada 300ms (Evita saturar el bus I2C)
        if(millis() - ultimo_refresco > 300) {

            float dist = getCurrentDistance();
            updateDisplay(IS_ANCHOR, dist);
            ultimo_refresco = millis();
        }

        // Dejamos que el bucle procese los nanosegundos del UWB a máxima velocidad 100 veces.
        // Solo entonces cedemos 1 milisegundo al procesador para evitar reinicios.
        if (contador_watchdog++ > 100) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            contador_watchdog = 0;
        }
    }
}

void taskRedes(void *pvParameters) {
    Serial.println("Core 0: Iniciando pila de red...");

    bool mqtt_iniciado = false;

    // Inicializamos el BLE con un nombre dinámico según el Rol
    if (IS_ANCHOR) {
        initWiFi(WIFI_SSID, WIFI_PASS);
    } else {

    }

    for(;;) {
        if (IS_ANCHOR) {
            if (isWiFiConnected()) {
                if (!mqtt_iniciado) {
                    ESP_LOGI(LOGTAG, "Wi-Fi estable. Iniciando MQTT hacia Gateway local...");
                    vTaskDelay(1500 / portTICK_PERIOD_MS);
                    
                    mqtt_app_start();
                    mqtt_iniciado = true;
                }
            } else {
                ESP_LOGW(LOGTAG, "Intentando reconectar...");
                reconnectWiFi(WIFI_SSID, WIFI_PASS);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

extern "C" void app_main() {
    initArduino();
    Serial.begin(115200);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    bleCommandQueue = xQueueCreate(5, 50 * sizeof(char));
        
    // Core 1: Prioridad alta (5) porque el UWB mide nanosegundos
    xTaskCreatePinnedToCore(taskUWB, "Tarea_UWB", 8192, NULL, 5, NULL, 1);

    // Core 0: Prioridad estándar (2) para manejar buffers de red    
    xTaskCreatePinnedToCore(taskRedes, "Tarea_Redes", 8192, NULL, 2, NULL, 0);
}