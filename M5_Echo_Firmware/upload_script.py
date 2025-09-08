# upload_script.py
import os
import subprocess
import sys

Import("env")

def upload_ota(source, target, env):
    # Fest kodierte IP-Adresse für OTA (verhindert PlatformIOs automatische Erkennung)
    upload_port = "10.0.4.107"
    
    # Pfad zur Firmware-Datei
    firmware_path = str(source[0])
    
    ota_cmd = [
        "curl", "-v", "-F",
        f"update=@{firmware_path}",
        f"http://{upload_port}/update"
    ]

    print(f"🚀 --- Versuch des OTA-Uploads an {upload_port} ---")
    try:
        result = subprocess.run(ota_cmd, check=True, capture_output=True, text=True, timeout=10)
        print("✅ --- OTA-Upload erfolgreich! ---")
        print(result.stdout)
        return True
    except subprocess.TimeoutExpired:
        print("⚠️ --- OTA-Upload fehlgeschlagen: Timeout (Gerät nicht im Netzwerk?) ---")
        return False
    except subprocess.CalledProcessError as e:
        print("⚠️ --- OTA-Upload fehlgeschlagen! ---")
        if e.stderr:
            print("Fehlerdetails:\n" + e.stderr)
        return False
    except FileNotFoundError:
        print("❌ Fehler: 'curl' wurde nicht gefunden. Ist es installiert und im System-PATH?")
        return False

def upload_serial(source, target, env):
    print("🔌 --- Führe Fallback auf seriellen Upload aus... ---")
    
    # Führe den Standard esptool Upload aus
    firmware_path = str(source[0])
    upload_cmd = [
        env.subst("$PYTHONEXE"),
        env.subst("$UPLOADER"),
        "--chip", "esp32",
        "--port", "/dev/cu.usbserial-61525036F0",
        "--baud", "115200",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash", "-z",
        "--flash_mode", "qio",
        "--flash_freq", "80m",
        "--flash_size", "4MB",
        "0x1000", env.subst("$BUILD_DIR/bootloader.bin"),
        "0x8000", env.subst("$BUILD_DIR/partitions.bin"),
        "0x10000", firmware_path
    ]
    
    try:
        result = subprocess.run(upload_cmd, check=True)
        print("✅ --- Serieller Upload erfolgreich! ---")
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ --- Serieller Upload fehlgeschlagen: {e} ---")
        return False

def intelligent_upload(source, target, env):
    print("🚀 === INTELLIGENTER UPLOAD-PROZESS GESTARTET ===")
    
    if upload_ota(source, target, env):
        print("🎉 === UPLOAD ERFOLGREICH ÜBER OTA ===")
        return
    else:
        print("🔄 --- OTA fehlgeschlagen, verwende seriellen Fallback ---")
        if upload_serial(source, target, env):
            print("🎉 === UPLOAD ERFOLGREICH ÜBER USB ===")
        else:
            print("❌ === UPLOAD FEHLGESCHLAGEN ===")
            env.Exit(1)

# Registriere unsere Upload-Funktion als custom upload command
env.Replace(UPLOADCMD=intelligent_upload)