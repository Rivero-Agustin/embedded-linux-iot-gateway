#!/usr/bin/env python3
"""
UWB Dataset Collector for Edge AI / TinyML
==========================================
Herramienta interactiva para capturar, etiquetar y exportar series temporales
del sensor Decawave DW1000 a formato CSV compatible con Edge Impulse.

Autor: Agustin Rivero
Proyecto: Embedded Linux IoT Gateway - Collision Avoidance
"""

import sys
import os
import time
import csv
import glob

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("❌ Error: 'pyserial' no está instalado.")
    print("👉 Instálalo ejecutando: pip install pyserial")
    sys.exit(1)

# Configuración por defecto
DEFAULT_BAUD = 115200
DEFAULT_DURATION = 4.0  # Duración de cada muestra en segundos
DATASET_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "dataset")

CLASSES = {
    "1": ("static_safe", "Estático / Seguro (> 3 metros o sin movimiento)"),
    "2": ("pedestrian_approach", "Peatón caminando hacia el sensor (~1 m/s)"),
    "3": ("vehicle_hazard", "Vehículo / Acercamiento rápido (> 2.5 m/s)"),
    "4": ("nlos_noise", "Ruido / Obstrucción NLOS (cajas, manos, cuerpo)")
}


def list_serial_ports():
    """Lista todos los puertos COM / tty disponibles."""
    ports = serial.tools.list_ports.comports()
    return [p.device for p in ports]


def select_port():
    """Permite al usuario seleccionar el puerto serie."""
    ports = list_serial_ports()
    if not ports:
        print("⚠️ No se detectaron puertos serie activos.")
        return input("Ingresa el puerto manualmente (ej. COM3 o /dev/ttyUSB0): ").strip()
    
    if len(ports) == 1:
        print(f"🔌 Puerto detectado automáticamente: {ports[0]}")
        use_auto = input(f"¿Usar {ports[0]}? (S/n): ").strip().lower()
        if use_auto in ("", "s", "si", "y", "yes"):
            return ports[0]

    print("\nPuertos disponibles:")
    for idx, p in enumerate(ports):
        print(f"  [{idx + 1}] {p}")
    
    while True:
        choice = input(f"Selecciona un puerto [1-{len(ports)}]: ").strip()
        if choice.isdigit() and 1 <= int(choice) <= len(ports):
            return ports[int(choice) - 1]
        print("Opción inválida. Intenta de nuevo.")


def count_existing_samples(dataset_dir):
    """Cuenta cuántas muestras hay guardadas por cada clase."""
    counts = {}
    for key, (label, _) in CLASSES.items():
        folder = os.path.join(dataset_dir, label)
        if os.path.exists(folder):
            files = glob.glob(os.path.join(folder, "*.csv"))
            counts[label] = len(files)
        else:
            counts[label] = 0
    return counts


def print_status(dataset_dir):
    """Muestra el resumen actual del dataset."""
    counts = count_existing_samples(dataset_dir)
    print("\n" + "=" * 55)
    print("📊 RESUMEN ACTUAL DEL DATASET:")
    for key, (label, desc) in CLASSES.items():
        c = counts.get(label, 0)
        status_icon = "🟢" if c >= 15 else "🟡" if c > 0 else "⚪"
        print(f"  {status_icon} [{key}] {label:20s}: {c:2d} muestras  ({desc})")
    print("=" * 55)


def record_sample(ser, label, duration=DEFAULT_DURATION):
    """Graba una muestra durante 'duration' segundos y la guarda en CSV."""
    folder = os.path.join(DATASET_DIR, label)
    os.makedirs(folder, exist_ok=True)
    
    # Encontrar el siguiente índice disponible
    existing = glob.glob(os.path.join(folder, f"{label}_*.csv"))
    idx = len(existing) + 1
    filename = os.path.join(folder, f"{label}_{idx:03d}.csv")

    print(f"\n🎬 Preparado para grabar clase: [{label}]")
    print(f"⏱️  Duración: {duration} segundos.")
    input("👉 Presiona ENTER cuando estés listo para empezar a mover el sensor...")

    # Cuenta regresiva
    for i in range(3, 0, -1):
        print(f"   ⏳ {i}...", end="\r", flush=True)
        time.sleep(0.5)
    print("   🔴 ¡GRABANDO! Realiza el movimiento ahora...       ")

    # Limpiar buffer de entrada para asegurar lecturas frescas
    ser.reset_input_buffer()

    samples = []
    start_time = None
    first_raw_time = None
    record_end = time.time() + duration

    while time.time() < record_end:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not line.startswith("DATA,"):
            continue

        parts = line.split(",")
        if len(parts) != 5:
            continue

        try:
            raw_timestamp_ms = int(parts[1])
            distance_m = float(parts[2])
            rx_power_dbm = float(parts[3])
            fp_power_dbm = float(parts[4])

            if first_raw_time is None:
                first_raw_time = raw_timestamp_ms

            # Normalizar timestamp relativo a t=0 ms
            rel_timestamp = raw_timestamp_ms - first_raw_time

            samples.append({
                "timestamp": rel_timestamp,
                "distance": distance_m,
                "rx_power": rx_power_dbm,
                "fp_power": fp_power_dbm
            })

            elapsed = duration - (record_end - time.time())
            print(f"   📊 [{elapsed:4.1f}s] Dist: {distance_m:5.2f}m | RX: {rx_power_dbm:6.1f}dBm | FP: {fp_power_dbm:6.1f}dBm", end="\r", flush=True)

        except (ValueError, IndexError):
            continue

    print("\n   ⏹️  Grabación finalizada.                                 ")

    if len(samples) < 5:
        print("❌ Error: No se recibieron suficientes datos del sensor. Verifica que el ESP32 esté encendido y midiendo.")
        return False

    # Guardar en formato CSV compatible con Edge Impulse
    with open(filename, mode="w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "distance", "rx_power", "fp_power"])
        for s in samples:
            writer.writerow([s["timestamp"], s["distance"], s["rx_power"], s["fp_power"]])

    # Resumen de la muestra
    dists = [s["distance"] for s in samples]
    print(f"✅ Guardado con éxito: {os.path.relpath(filename)}")
    print(f"   📈 Muestras capturadas: {len(samples)} filas | Dist mín: {min(dists):.2f}m | Dist máx: {max(dists):.2f}m")
    return True


def live_monitor(ser):
    """Muestra los datos en vivo del sensor para verificar antes de grabar."""
    print("\n🔍 Modo Monitor en Vivo (presiona Ctrl+C para volver al menú):")
    print("-" * 55)
    ser.reset_input_buffer()
    try:
        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith("DATA,"):
                parts = line.split(",")
                if len(parts) == 5:
                    print(f"  📡 T: {parts[1]:>8}ms | Dist: {float(parts[2]):5.2f}m | RX: {float(parts[3]):6.1f}dBm | FP: {float(parts[4]):6.1f}dBm")
            elif line:
                print(f"  [LOG] {line}")
    except KeyboardInterrupt:
        print("\n🔙 Volviendo al menú principal...")


def main():
    print("=======================================================")
    print("  🧠 UWB Edge AI Data Collector - Decawave DW1000")
    print("=======================================================")

    port = select_port()
    try:
        ser = serial.Serial(port, DEFAULT_BAUD, timeout=1.0)
        time.sleep(1.5)  # Esperar estabilización del puerto
        print(f"✅ Conectado a {port} a {DEFAULT_BAUD} baudios.")
    except Exception as e:
        print(f"❌ No se pudo abrir el puerto {port}: {e}")
        sys.exit(1)

    try:
        while True:
            print_status(DATASET_DIR)
            print("\nOpciones:")
            for key, (label, desc) in CLASSES.items():
                print(f"  [{key}] Grabar muestra para: {label}")
            print("  [M] Monitor en vivo (ver lecturas en tiempo real)")
            print("  [Q] Salir")

            choice = input("\nElige una opción: ").strip().lower()

            if choice == "q":
                print("👋 ¡Hasta luego!")
                break
            elif choice == "m":
                live_monitor(ser)
            elif choice in CLASSES:
                label, _ = CLASSES[choice]
                record_sample(ser, label)
            else:
                print("⚠️ Opción no válida.")

    finally:
        ser.close()
        print("🔌 Conexión serie cerrada.")


if __name__ == "__main__":
    main()

