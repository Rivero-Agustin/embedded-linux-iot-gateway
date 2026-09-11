#include <unity.h>

// Esta es la función que vamos a probar (podría ser tu algoritmo de UWB)
int calcular_riesgo_colision(int distancia) {
    if (distancia < 100) return 1; // 1 = Peligro
    return 0; // 0 = Seguro
}

// Este es el test que evalúa si la función hace lo correcto
void test_algoritmo_colision(void) {
    TEST_ASSERT_EQUAL(1, calcular_riesgo_colision(50));
    TEST_ASSERT_EQUAL(0, calcular_riesgo_colision(150));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_algoritmo_colision);
    UNITY_END();
    return 0;
}