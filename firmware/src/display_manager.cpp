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
    display.setCursor(0, 5);
    display.println("Sistema UWB Activo");
    display.drawLine(0, 15, 128, 15, SSD1306_WHITE);
    display.display();
}

void updateDisplay(bool isAnchor, float distance){
// Limpiamos SOLO nuestra área inferior de texto para evitar parpadeos
    display.fillRect(0, 20, SCREEN_WIDTH, SCREEN_HEIGHT - 20, SSD1306_BLACK);
    
    display.setCursor(0, 25);
    display.println(isAnchor ? "Rol: ANCLA" : "Rol: TAG");
    
    display.setCursor(0, 45);
    display.setTextSize(2); 
    display.printf("%.2f m", distance);
    display.setTextSize(1);
    display.display();
}