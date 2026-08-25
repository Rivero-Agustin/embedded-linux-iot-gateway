# UWB Collision Avoidance System: Edge to AWS

Sistema IoT de detección de colisiones utilizando tecnología UWB (Ultra-Wideband) con ESP32, un Gateway local (Edge Computing) emulado en Linux/QEMU, y conexión segura a AWS IoT Core.

## 🏗️ Arquitectura del Sistema

````mermaid
flowchart LR
    %% Definición de estilos
    classDef hardware fill:#f9f9f9,stroke:#333,stroke-width:2px;
    classDef windows fill:#e1f5fe,stroke:#0288d1,stroke-width:2px;
    classDef wsl fill:#fff3e0,stroke:#f57c00,stroke-width:2px;
    classDef embedded fill:#e8f5e9,stroke:#388e3c,stroke-width:2px;
    classDef aws fill:#fff9c4,stroke:#fbc02d,stroke-width:2px;

    subgraph Hardware ["Frontera Física (Hardware)"]
        Tag([Tag UWB]) -- "RF (Medición)" --> Anchor([Anchor UWB <br> ESP32])
    end

    subgraph PC ["Host (Windows)"]
        FW[Firewall <br> Regla TCP 1883]
        Proxy[Portproxy <br> netsh v4tov4]
        Anchor -- "Wi-Fi (MQTT unencrypted)" --> FW
        FW --> Proxy
    end

    subgraph WSL ["Virtualización (WSL2)"]
        Ubuntu[Red Interna <br> Ubuntu eth0]
        Proxy -- "Túnel" --> Ubuntu
    end

    subgraph QEMU ["Embedded Linux (Buildroot / QEMU)"]
        Mosquitto[(Mosquitto <br> Broker Local)]
        PythonBridge{gateway.py <br> Lógica Edge}
        Ubuntu -- "hostfwd <br> (0.0.0.0:1883)" --> Mosquitto
        Mosquitto -- "JSON (Local)" --> PythonBridge
    end

    subgraph Cloud ["Nube (AWS)"]
        AWSIoT((AWS IoT Core))
        PythonBridge -- "TLS/MQTTS <br> Puerto 8883" --> AWSIoT
    end

    %% Asignación de clases
    Tag:::hardware
    Anchor:::hardware
    FW:::windows
    Proxy:::windows
    Ubuntu:::wsl
    Mosquitto:::embedded
    PythonBridge:::embedded
    AWSIoT:::aws

El proyecto se divide en tres capas principales:
1. **Percepción (Hardware):** Nodos ESP32 (Tags y Anchors) con módulos UWB midiendo distancias físicas.
2. **Edge Computing (Gateway):** Un entorno Linux ligero (Buildroot) emulado en QEMU. Ejecuta un script en Python que procesa la telemetría, aplica reglas de negocio (detección de anomalías/colisiones) y actúa como puente.
3. **Nube (AWS IoT Core):** Recepción de alertas críticas mediante MQTT sobre TLS (MQTTS).

## 📁 Estructura del Repositorio

- `/firmware`: Código fuente C++ para el ESP32 (Anchor y Tag).
- `/gateway`: Script principal de Python `gateway.py` que corre en el entorno Linux.
- `README.md`: Documentación del proyecto.

> **⚠️ Nota de Seguridad:** Los certificados de AWS (`.pem`, `.key`, `.crt`) necesarios para el Gateway NO están incluidos en este repositorio por seguridad. Deben generarse en AWS IoT Core y ubicarse en el entorno QEMU.

## 🚀 Guía de Despliegue Local (Entorno Windows/WSL2)

Debido a que el Gateway se ejecuta en QEMU dentro de WSL2 (Windows Subsystem for Linux), es necesario configurar el enrutamiento de red para que el ESP32 físico pueda comunicarse con el broker MQTT emulado.

### 1. Configuración de Red en Windows (Portproxy)
WSL2 asigna una IP dinámica en cada reinicio. Para que el ESP32 alcance a QEMU, debemos crear un túnel.

Abrir **PowerShell como Administrador** y obtener la IP de WSL:
```powershell
wsl -e hostname -i

Crear el túnel (reemplazar <IP_WSL> por la obtenida):

PowerShell
netsh interface portproxy add v4tov4 listenport=1883 listenaddress=0.0.0.0 connectport=1883 connectaddress=<IP_WSL>
2. Configuración del Firewall de Windows
Para permitir la entrada de telemetría desde el ESP32:

Abrir Firewall de Windows Defender con seguridad avanzada.

Crear una Nueva Regla de Entrada -> Puerto -> TCP -> 1883.

Permitir conexión.

Importante: Marcar solo los perfiles Dominio y Privado (Dejar "Público" desmarcado para evitar vulnerabilidades en redes abiertas).

3. Ejecución del Gateway (QEMU)
Dentro de la terminal de Ubuntu (WSL), levantar el sistema emulado asegurando que el reenvío de puertos esté configurado (hostfwd=tcp:0.0.0.0:1883-:1883 en el script de arranque).

Una vez dentro de QEMU, iniciar el broker y el script puente:

Bash
# Iniciar broker MQTT local en segundo plano (si no inicia automáticamente)
mosquitto -d

# Ejecutar el puente hacia AWS
python3 gateway.py
4. Configuración del ESP32
En el código del ESP32, modificar el archivo de configuración para apuntar a la IP IPv4 local de la máquina Windows (Ej: 192.168.1.X):

C++
mqtt_cfg.uri = "mqtt://<IP_WINDOWS>:1883";
Compilar y flashear el código al ESP32 usando ESP-IDF o PlatformIO.

📡 Tópicos MQTT
Telemetría Local (ESP32 -> Gateway): gateway/uwb/telemetry

Alertas Cloud (Gateway -> AWS): gateway/uwb/alerts

⚙️ Reglas de Detección de Colisiones (Edge)
El Gateway evalúa los datos localmente para reducir la latencia y el ancho de banda hacia la nube:

Peligro Sostenido: Si la distancia es menor a 2.0m durante 3 lecturas consecutivas.

Salto Brusco (Anomalía): Si la diferencia entre dos lecturas consecutivas es mayor a 5.0m (filtro de ruido del sensor).
````
