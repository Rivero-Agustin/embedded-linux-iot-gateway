#include <unity.h>

// 1. La Lógica Pura que estamos testeando
int calcular_riesgo_colision(int distancia) {
    if (distancia < 100) return 1;
    return 0;
}

void test_algoritmo_colision(void) {
    TEST_ASSERT_EQUAL(1, calcular_riesgo_colision(50));
    TEST_ASSERT_EQUAL(0, calcular_riesgo_colision(150));
}

// ==========================================
// MAGIA MULTI-ENTORNO (Cloud vs Hardware)
// ==========================================

#if defined(ESP_PLATFORM) || defined(ARDUINO)
// ---> Si estamos flasheando el ESP32 (Hardware-in-the-loop con ESP-IDF / Arduino)
#include <Arduino.h>

extern "C" void app_main() {
    #if defined(ARDUINO)
    initArduino();
    #endif
    
    // Esperar 2 segundos para que el puerto Serial se estabilice tras el reinicio
    vTaskDelay(2000 / portTICK_PERIOD_MS); 

    UNITY_BEGIN();
    RUN_TEST(test_algoritmo_colision);
    UNITY_END();
}

#else
// ---> Si estamos compilando en los servidores de GitHub (Ubuntu Native)
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_algoritmo_colision);
    UNITY_END();
    return 0;
}
#endif