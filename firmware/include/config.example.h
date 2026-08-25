#ifndef CONFIG_H
#define CONFIG_H

// --- CONFIGURACIÓN DE ROL (Ancla o Etiqueta) ---
#define IS_ANCHOR true

/* =======================================================
 * CREDENCIALES DE RED Y BROKER LOCAL (GATEWAY)
 * (Reemplazar con datos reales)
 * ======================================================= */
#define WIFI_SSID "TU_RED_WIFI"
#define WIFI_PASS "TU_CONTRASEÑA"

// Dirección IP del Gateway local (tu PC / Linux QEMU con hostfwd)
// Puerto 1883 sin TLS/SSL
#define MQTT_BROKER_URI "mqtt://192.168.1.X:1883"

// Tópicos MQTT locales
#define MQTT_TOPIC_TELEMETRY "gateway/uwb/local"
#define MQTT_TOPIC_COMMANDS  "gateway/uwb/commands"

#endif // CONFIG_H