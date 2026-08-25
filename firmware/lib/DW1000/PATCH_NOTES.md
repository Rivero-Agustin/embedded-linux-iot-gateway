# Modificaciones a la librería DW1000 (Makerfabs Fork)

Esta librería fue modificada para el proyecto de RTLS 2D.
No actualizar desde el gestor de librerías de PlatformIO.

## Modificaciones @CUSTOM_PATCH (Julio 2026):

1. **Soporte de Texto UWB (Custom Payload):**
   - _DW1000Ranging.h_: Agregada función pública `transmitCustomData()` y puntero privado `_handleCustomData`.
   - _DW1000Ranging.cpp_:
     - Modificado `handleReceived()` para interceptar el byte `0x80` y derivarlo a un callback en lugar de procesarlo como cálculo de tiempo de vuelo.
     - Declaracion del callback `attachCustomDataReceived()` para devolver el dato al programa.
