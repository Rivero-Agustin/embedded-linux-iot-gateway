#include "nvs_flash.h"
#include "esp_log.h"

static const char *LOGTAG = "NVS_MANAGER";

// Inicializar la partición NVS para configuración básica del sistema
void init_nvs() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(LOGTAG, "NVS inicializado correctamente");
}
