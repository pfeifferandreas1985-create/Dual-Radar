# TRON Dual Radar System 📡

![Project Status](https://img.shields.io/badge/Status-Functional-brightgreen) ![Platform](https://img.shields.io/badge/Platform-ESP8266-blue) ![License](https://img.shields.io/badge/License-MIT-orange)

Ein fortschrittliches, duales Radarsystem, das die Präzision von **LIDAR (TOF)** mit der flächendeckenden Sicherheit von **Ultraschall (Sonar)** kombiniert. Visualisiert in einem futuristischen "TRON-Style" Command Center.

---

## 🌟 Features

### 1. Compound-Radar (Omni-Sonar)
*   **360° Rundumsicht:** 4x HC-SR04 Sensoren (Vorne, Hinten, Links, Rechts).
*   **High-Speed Scan:** Sektor-Optimierung (40°-140°) ermöglicht doppelte Scan-Geschwindigkeit bei voller Abdeckung.
*   **Map Mode:** Erstellt eine persistente Karte des Raumes (SLAM-ähnliche Visualisierung).
*   **Interleaved Sampling:** Intelligente Sensor-Abfrage verhindert Interferenzen ("Crosstalk").
*   **Hardware-Fix:** Spezieller WiFi-Modus verhindert Servo-Zittern.

### 2. LIDAR-Radar (Precision Scope)
*   **Technologie:** VL53L1X Time-of-Flight Laser.
*   **Präzision:** Millimeter-genaue Messung für feine Details (Tischbeine, Türspalte).
*   **Visualisierung:** 180° Scanner mit "Nachleucht"-Effekt (Phosphor-Trail).

### 3. Unified Command Center (Dashboard)
*   **Dual-View:** Gleichzeitige Darstellung beider Radare auf getrennten Scopes.
*   **Live-Daten:** Echtzeit-Anzeige aller Sensorwerte (cm/mm).
*   **Steuerung:** Geschwindigkeits-Slider für das Omni-Radar.
*   **Technologie:** HTML5 Canvas & WebSockets (Keine Installation nötig, läuft im Browser).

---

## 📂 Projektstruktur

```
Radar/
├── Docs/               # Dokumentation & Notizen
├── Firmware/           # Arduino/ESP8266 Code
│   ├── CompoundRadar/  # Firmware für das 360° Ultraschall-System
│   └── LidarRadar/     # Firmware für das TOF-System
├── Hardware/           # 3D-Druck & CAD
│   └── CAD/            # .STL, .3MF und Source-Files (SolidWorks)
└── Software/
    └── Dashboard/      # Das Web-Interface (TRON_Radar.html)
```

---

## 🚀 Installation & Setup

### 1. Hardware
*   **Mikrocontroller:** 2x ESP8266 (NodeMCU oder Wemos D1 Mini).
*   **Verkabelung Compound:**
    *   Servo: D4 (GPIO 2)
    *   Trigger A (Front/Back): D5 (GPIO 14)
    *   Trigger B (Left/Right): D6 (GPIO 12)
    *   Echo Pins: D1, D2, D7, D8
*   **Verkabelung LIDAR:**
    *   Servo: D4 (GPIO 2)
    *   VL53L1X: I2C (D1/D2)

### 2. Firmware Flashen
1.  Öffne die `.ino` Dateien in der Arduino IDE.
2.  Installiere benötigte Bibliotheken: `Servo`, `WebSocketsServer`, `VL53L1X` (für Lidar).
3.  Passe ggf. WLAN-SSID/Passwort im Code an.
4.  Flashe die ESPs.

### 3. Dashboard Starten
1.  Verbinde deinen PC mit dem WLAN der ESPs (oder sorge dafür, dass alle im gleichen Heimnetz sind).
2.  Öffne `Software/Dashboard/TRON_Radar.html` in einem modernen Browser (Chrome/Firefox).
3.  Gib die IP-Adressen der beiden Radare ein und klicke auf **INITIALIZE**.

---

## 🛠️ Steuerung

*   **SPEED Slider:** Regelt die Scan-Geschwindigkeit des Ultraschall-Radars.
*   **RESET MAP:** Löscht die gezeichnete Karte.
*   **HUD:** Zeigt Live-Werte für Front, Back, Left, Right (Sonar) und Distanz (Lidar).

---

## 🤝 Contributing
Pull Requests sind willkommen! Für größere Änderungen bitte erst ein Issue öffnen.

**Author:** Andreas Pfeiffer
**Co-Pilot:** Google DeepMind Agent
