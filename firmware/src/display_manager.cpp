#include "display_manager.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_log.h"

// Configuración de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

static const char* LOGTAG = "DISPLAY";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initDisplay() {
    Wire.begin(I2C_SDA, I2C_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        ESP_LOGE(LOGTAG, "Fallo al inicializar la pantalla OLED.");
        vTaskDelete(NULL); // Destruir la tarea si falla
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 3);
    display.println("UWB + TinyML AI Edge");
    display.drawLine(0, 13, 128, 13, SSD1306_WHITE);
    display.display();
}

void updateDisplay(bool isAnchor, float distance, const char* ai_label, float confidence){
    // Limpiamos el área inferior para actualizar sin parpadeo del encabezado
    display.fillRect(0, 15, SCREEN_WIDTH, SCREEN_HEIGHT - 15, SSD1306_BLACK);
    
    // Fila 1: Rol y Distancia
    display.setCursor(0, 18);
    display.setTextSize(1);
    display.printf("%s | %.2fm", isAnchor ? "ANCLA" : "TAG", distance);
    
    // Fila 2: Estado predicho por la IA (Resaltado o grande)
    display.setCursor(0, 32);
    display.setTextSize(1);
    if (strcmp(ai_label, "vehicle_hazard") == 0) {
        display.println("ALERTA: VEHICULO");
    } else if (strcmp(ai_label, "pedestrian_approach") == 0) {
        display.println("PEATON ACERCANDOSE");
    } else if (strcmp(ai_label, "nlos_noise") == 0) {
        display.println("RUIDO / NLOS");
    } else if (strcmp(ai_label, "static_safe") == 0) {
        display.println("ESTADO: SEGURO");
    } else {
        display.printf("AI: %s\n", ai_label);
    }
    
    // Fila 3: Nivel de Confianza
    display.setCursor(0, 48);
    display.setTextSize(1);
    display.printf("Confianza: %.0f%%", confidence * 100.0f);

    display.display();
}