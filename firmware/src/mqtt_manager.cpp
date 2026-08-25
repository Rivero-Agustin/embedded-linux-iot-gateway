#include "mqtt_manager.h"
#include "uwb_engine.h"
#include "nvs_manager.h"
#include "config.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "cJSON.h"
#include <string.h>

static const char* LOGTAG = "MQTT";

// Variables globales para la tarea
TaskHandle_t telemetry_task_handle = NULL;
esp_mqtt_client_handle_t global_mqtt_client = NULL;

// Handler que procesa todos los eventos de la conexión MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // Cambiamos %ld por %d porque event_id es int32_t
    ESP_LOGD(LOGTAG, "Evento despachado desde el bucle base=%s, event_id=%d", base, (int)event_id);
    
    // C++ requiere casting explícito desde void*
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id) {
        
        case MQTT_EVENT_CONNECTED: {
            ESP_LOGI(LOGTAG, "Conectado al Gateway MQTT local con éxito.");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_COMMANDS, 1);
    
            // Lanzamos la tarea de FreeRTOS si no estaba corriendo ya
            if (telemetry_task_handle == NULL) {
                xTaskCreate(
                    uwb_telemetry_task,      // Puntero a la función de la tarea
                    "uwb_telemetry_task",    // Nombre para debug
                    4096,                    // Tamaño del Stack (En bytes para ESP-IDF)
                    NULL,                    // Parámetros
                    5,                       // Prioridad (5 es estándar/alta)
                    &telemetry_task_handle   // Manejador
                );
            }
            break;
        } 
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(LOGTAG, "MQTT_EVENT_DISCONNECTED: Se perdió la conexión con el Gateway");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(LOGTAG, "MQTT_EVENT_SUBSCRIBED: Suscripción confirmada, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(LOGTAG, "MQTT_EVENT_PUBLISHED: Publicación confirmada, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(LOGTAG, "MQTT_EVENT_DATA: Mensaje recibido desde el Gateway");
            printf("Tópico: %.*s\r\n", event->topic_len, event->topic);
            printf("Datos: %.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(LOGTAG, "MQTT_EVENT_ERROR: Error en la conexión");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(LOGTAG, "Error del socket TCP: %s", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;

        default:
            ESP_LOGD(LOGTAG, "Evento MQTT no manejado, id: %d", event->event_id);
            break;
    }
}

void mqtt_app_start(void)
{
    // BARRERA DE SEGURIDAD: Si es un Tag, abortamos la inicialización de MQTT para ahorrar batería
    if (!IS_ANCHOR) {
        ESP_LOGI(LOGTAG, "Rol: TAG. Se deshabilita el cliente MQTT para ahorrar energía.");
        return;
    }

    // Configuración para broker MQTT local (sin TLS/certificados)
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.uri = MQTT_BROKER_URI;
    mqtt_cfg.buffer_size = 1024;
    mqtt_cfg.out_buffer_size = 1024;

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    global_mqtt_client = client;
}