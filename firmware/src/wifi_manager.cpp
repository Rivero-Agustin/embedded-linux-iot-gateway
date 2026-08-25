#include "wifi_manager.h"
#include "mqtt_manager.h"

#include "esp_log.h"
#include <WiFi.h>

static const char* LOGTAG = "WIFI";

void initWiFi(const char* ssid, const char* password) {
    ESP_LOGI(LOGTAG, "Iniciando conexión Wi-Fi...");

    // Configura el ESP32 en modo estación (cliente)
    WiFi.mode(WIFI_STA); 
    WiFi.begin(ssid, password);

    // Intentamos conectar de forma asíncrona (le damos un límite de intentos)
    int intentos = 0;
    const int max_intentos = 20; // 10 segundos máximo
    
    while (WiFi.status() != WL_CONNECTED && intentos < max_intentos) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        ESP_LOGW(LOGTAG, "Intento de conexión %d/%d", intentos + 1, max_intentos);
        intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(LOGTAG, "Wi-Fi conectado con éxito. IP: %s", WiFi.localIP().toString().c_str());
    } else {
        ESP_LOGE(LOGTAG, "No se pudo conectar al Wi-Fi");
    }
}

bool isWiFiConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

String getWiFiIP() {
    if (isWiFiConnected()) {
        return WiFi.localIP().toString();
    }
    return "Desconectado";
}

void reconnectWiFi(const char* ssid, const char* password) {
    // Desconectamos cualquier enlace basura previo por seguridad
    WiFi.disconnect(); 
    // Intentamos levantar la conexión nuevamente
    WiFi.begin(ssid, password);
}