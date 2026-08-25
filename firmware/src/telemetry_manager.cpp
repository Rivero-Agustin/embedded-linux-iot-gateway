#include "cJSON.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

static const char *LOGTAG = "TELEMETRY";

// Función para construir y publicar el JSON localmente al Gateway
void publish_uwb_telemetry(esp_mqtt_client_handle_t client, const char* tag_id, float distance_m) {
    if (client == NULL) return;

    // 1. Obtener la MAC del ESP32 para identificar este nodo (Ancla)
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char anchor_id[18];
    snprintf(anchor_id, sizeof(anchor_id), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // 2. Crear el objeto JSON base
    cJSON *root = cJSON_CreateObject();
    
    // 3. Poblar el JSON con la telemetría
    cJSON_AddStringToObject(root, "anchor_id", anchor_id);
    cJSON_AddStringToObject(root, "tag_id", tag_id);
    cJSON_AddNumberToObject(root, "distance_m", distance_m);

    // 4. Convertir a string sin formato (ocupa menos bytes que Print normal)
    char *json_string = cJSON_PrintUnformatted(root);

    // 5. Publicar en el tópico local configurado para el Gateway
    int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC_TELEMETRY, json_string, 0, 1, 0);
    
    if (msg_id != -1) {
        ESP_LOGI(LOGTAG, "Telemetría local publicada en '%s' (msg_id=%d): %s", MQTT_TOPIC_TELEMETRY, msg_id, json_string);
    } else {
        ESP_LOGE(LOGTAG, "Error publicando telemetría local");
    }

    // 6. ¡CRÍTICO! Liberar la memoria RAM del Heap
    cJSON_Delete(root);   // Libera el árbol JSON
    free(json_string);    // Libera el string generado por cJSON_Print
}