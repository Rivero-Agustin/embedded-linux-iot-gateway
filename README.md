# Custom Embedded Linux Edge Gateway: Buildroot & QEMU

<!-- Technology Badges -->

![Linux](https://img.shields.io/badge/Embedded_Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Python](https://img.shields.io/badge/Python-3670A0?style=for-the-badge&logo=python&logoColor=ffdd54)
![C++](https://img.shields.io/badge/C++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/ESP32-E7352C?style=for-the-badge&logo=espressif&logoColor=white)
![MQTT](https://img.shields.io/badge/MQTT-660066?style=for-the-badge&logo=mqtt&logoColor=white)
![AWS](https://img.shields.io/badge/AWS_IoT_Core-%23FF9900.svg?style=for-the-badge&logo=amazon-aws&logoColor=white)

IoT collision avoidance system using UWB (Ultra-Wideband) technology with ESP32, a local Edge Computing Gateway emulated on Linux/QEMU, and a secure connection to AWS IoT Core.

> 🇪🇸 **Looking for the Spanish version?** Check out [README-es.md](./README-es.md).

---

## 🏗️ System Architecture

![System Architecture Diagram](./docs/architecture.diagram.png)

The project is divided into three main layers:

1. **Perception (Hardware):** ESP32 nodes (Tags and Anchors) with UWB modules measuring physical distances.
2. **Edge Computing (Gateway):** A lightweight Linux environment (Buildroot) emulated on QEMU. It runs a Python script that processes telemetry, applies business logic (anomaly and collision detection), and acts as a bridge.
3. **Cloud (AWS IoT Core):** Reception of critical alerts via MQTT over TLS (MQTTS).

---

## 📁 Repository Structure

- `firmware/`: C++ source code for the ESP32 (Anchor and Tag using PlatformIO).
- `gateway/`: Main Python script `gateway.py` running on the Linux environment.
- `README.md`: English documentation and deployment guide.
- `README-es.md`: Documentación en español.

> [!WARNING]
> **Security Note:** AWS certificates (`.pem`, `.key`, `.crt`) required for the Gateway are **NOT** included in this repository for security reasons. They must be generated in AWS IoT Core and placed in the Linux/QEMU environment (`/root/certs`).

---

## 🚀 Local Deployment Guide (Windows / WSL2 Environment)

Since the Gateway runs in QEMU within WSL2 (Windows Subsystem for Linux), network routing needs to be configured so that the physical ESP32 can communicate with the emulated MQTT broker.

### 1. Windows Network Configuration (Portproxy)

WSL2 assigns a dynamic IP on every reboot. To allow the ESP32 to reach QEMU, we must create a port proxy tunnel.

1. Open **PowerShell as Administrator** and retrieve the WSL IP:

   ```powershell
   wsl -e hostname -i
   ```

2. Create the tunnel (replace `<WSL_IP>` with the obtained IP):
   ```powershell
   netsh interface portproxy add v4tov4 listenport=1883 listenaddress=0.0.0.0 connectport=1883 connectaddress=<WSL_IP>
   ```

### 2. Windows Firewall Configuration

To allow inbound telemetry traffic from the ESP32:

1. Open **Windows Defender Firewall with Advanced Security**.
2. Create a **New Inbound Rule** -> **Port** -> **TCP** -> `1883`.
3. Select **Allow the connection**.
4. **Important:** Select only the _Domain_ and _Private_ profiles (leave _Public_ unchecked to prevent vulnerabilities on open networks).

### 3. Running the Gateway (QEMU)

Inside the Ubuntu (WSL) terminal, launch the emulated system ensuring port forwarding is configured (`hostfwd=tcp:0.0.0.0:1883-:1883` in the startup script).

Once inside QEMU, start the broker and the bridge script:

```bash
# Start local MQTT broker in the background (if not started automatically)
mosquitto -d

# Run the AWS bridge script
python3 gateway.py
```

### 4. ESP32 Firmware Configuration

1. Copy `firmware/include/config.example.h` to `firmware/include/config.h`:
   ```bash
   cp firmware/include/config.example.h firmware/include/config.h
   ```
2. Edit `config.h` with your network credentials and your Windows host local IP:
   ```c
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASS "YOUR_WIFI_PASSWORD"
   #define MQTT_BROKER_URI "mqtt://192.168.1.X:1883"
   ```
3. Build and flash the firmware to the ESP32 using **PlatformIO**.

---

## 📡 MQTT Topics

| Topic                   | Source ➔ Destination   | Protocol           | Description                                       |
| :---------------------- | :--------------------- | :----------------- | :------------------------------------------------ |
| `gateway/uwb/telemetry` | ESP32 ➔ Gateway        | MQTT (1883)        | Local telemetry with UWB distance measurements.   |
| `gateway/uwb/alerts`    | Gateway ➔ AWS IoT Core | MQTTS (8883 / TLS) | Critical proximity or anomaly alerts.             |

---

## ⚙️ Collision Detection Rules (Edge)

The Gateway evaluates data locally to reduce latency and cloud bandwidth consumption:

- **Sustained Danger:** Triggered if the measured distance is **< 2.0 m** for 3 consecutive readings.
- **Abrupt Jump (Anomaly):** Triggered if the difference between two consecutive readings is **> 5.0 m** (sensor noise filter and measurement glitch detection).
