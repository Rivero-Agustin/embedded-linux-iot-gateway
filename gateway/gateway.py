import paho.mqtt.client as mqtt
import json
import ssl

# --- CONFIGURACIÓN AWS ---
AWS_ENDPOINT = "a22o96apcs8yee-ats.iot.us-east-1.amazonaws.com"
CERTS_PATH = "/root/certs"

aws_client = mqtt.Client(client_id="Gateway-Linux-01")
aws_client.tls_set(
    ca_certs=f"{CERTS_PATH}/root-ca.pem",
    certfile=f"{CERTS_PATH}/cert.pem.crt",
    keyfile=f"{CERTS_PATH}/private.pem.key",
    tls_version=ssl.PROTOCOL_TLSv1_2
)

def on_aws_connect(client, userdata, flags, rc):
    print(f"[AWS] Conectado a la nube con código de estado: {rc}")

aws_client.on_connect = on_aws_connect

# --- MEMORIA Y PARÁMETROS DEL ALGORITMO ---
HISTORIAL = []
MAX_MUESTRAS = 5
UMBRAL_PELIGRO = 2.0

def detectar_anomalia(distancia):
    global HISTORIAL
    HISTORIAL.append(distancia)
    if len(HISTORIAL) > MAX_MUESTRAS:
        HISTORIAL.pop(0)

    # REGLA 1: Peligro sostenido
    if len(HISTORIAL) >= 3 and all(d < UMBRAL_PELIGRO for d in HISTORIAL[-3:]):
        alerta = {"alerta": "PELIGRO_SOSTENIDO", "distancia": distancia}
        print("[AWS] Enviando alerta de colisión a la nube...")
        aws_client.publish("gateway/uwb/alerts", json.dumps(alerta)) # Tópico genérico

   # REGLA 2: Salto brusco (ruido)
    if len(HISTORIAL) >= 2:
        salto = abs(HISTORIAL[-1] - HISTORIAL[-2])
        if salto > 5.0:
            alerta = {"alerta": "SALTO_BRUSCO", "diferencia": salto}
            print("[AWS] Enviando anomalía de sensor a la nube...")
            aws_client.publish("gateway/uwb/alerts", json.dumps(alerta)) # Tópico genérico

# --- LÓGICA MQTT LOCAL ---
def on_local_connect(client, userdata, flags, rc):
    print(f"[LOCAL] Conectado al broker Edge AI")
    client.subscribe("gateway/uwb/telemetry") # Tópico genérico

def on_local_message(client, userdata, msg):
    try:
        datos = json.loads(msg.payload.decode())
        # CORRECCIÓN: Usamos la misma clave que genera el cJSON del ESP32
        distancia = float(datos['distance_m'])
        print(f"-> Local reporta: {distancia}m")
        detectar_anomalia(distancia)
    except Exception as e:
        print(f"Error procesando dato local: {e}") # Para no sufrir en silencio

local_client = mqtt.Client()
local_client.on_connect = on_local_connect
local_client.on_message = on_local_message

print("Iniciando Gateway (Dual: Local + AWS)...")

aws_client.connect(AWS_ENDPOINT, 8883, 60)
aws_client.loop_start()

local_client.connect("127.0.0.1", 1883, 60)
local_client.loop_forever()