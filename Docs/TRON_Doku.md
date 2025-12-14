# TRON Radar Systems - Projektdokumentation

Diese Dokumentation fasst die Entwicklung der drei Systemkomponenten zusammen: das LIDAR-Radar, das Compound-Ultraschall-Radar und das vereinte TRON Command Center.

---

## 1. System A: Das LIDAR-Radar (TOF)
Ein hochpräzises Radar für feine Abtastungen im Nahbereich (bis 2m).

### Hardware
*   **Mikrocontroller:** ESP8266 (NodeMCU/Wemos D1 Mini)
*   **Sensor:** VL53L1X (Time-of-Flight Laser-Entfernungsmesser)
*   **Aktor:** Micro Servo (SG90 o.ä.)
*   **Verbindung:** I2C (SDA/SCL) für Sensor, PWM für Servo.

### Funktionsweise
*   **Scan-Bereich:** 180° (Halbkreis).
*   **Auflösung:** Millimeter-genau.
*   **Logik:** Der Servo schwenkt von 0° bis 180° und zurück. Bei jedem Schritt wird eine Laser-Messung durchgeführt.
*   **Datenübertragung:** Sendet JSON-Pakete (`{angle, distance}`) per WebSocket an den PC.

### Besonderheiten
*   Sehr präzise, aber nur ein einziger Messpunkt (Laserstrahl).
*   Visualisierung als "Scanner" mit verblassenden Punkten (roter "Schweif").

---

## 2. System B: Das Compound-Radar (Omni-Sonar)
Ein 360°-Rundumsicht-Radar für schnelle Raumerfassung.

### Hardware
*   **Mikrocontroller:** ESP8266
*   **Sensoren:** 4x HC-SR04 Ultraschall-Module.
*   **Aktor:** Micro Servo.
*   **Anordnung:** Die 4 Sensoren sind im 90°-Winkel zueinander montiert (Vorne, Hinten, Links, Rechts).

### Funktionsweise
*   **Sektor-Scan:** Der Servo dreht sich nur von **40° bis 140°** (100° Sektor).
*   **360° Abdeckung:** Da die Sensoren versetzt sind, deckt dieser kleine Schwenk den gesamten Raum ab (4x 90° = 360°).
*   **Interleaved Sampling:** Um Geschwindigkeit zu gewinnen, werden die Sensoren versetzt abgefragt (Front -> Links -> Back -> Rechts). Die Messzeit der einen Gruppe dient als Abkühlzeit der anderen.
*   **Geschwindigkeitsregelung:** Über einen Slider im Dashboard kann die Drehgeschwindigkeit (Pause zwischen Schritten) eingestellt werden.
*   **WiFi-Fix:** Nutzt `WiFi.setSleepMode(WIFI_NONE_SLEEP)`, um Servo-Zittern durch Stromspitzen zu verhindern.

### Besonderheiten
*   Erstellt eine **persistente Karte** (Map Mode) des Raumes.
*   Punkte bleiben dauerhaft stehen und bilden Wände als feine Linien (1px) ab.
*   Enorme Scan-Geschwindigkeit durch Sektor-Optimierung.

---

## 3. System C: Unified TRON Command Center
Die zentrale Steuereinheit, die beide Radarsysteme vereint.

### Technologie
*   **Plattform:** Web-basiert (HTML5, CSS3, JavaScript).
*   **Rendering:** Hardware-beschleunigtes Canvas API.
*   **Kommunikation:** Duale WebSocket-Clients (verbindet sich gleichzeitig mit beiden ESPs).
*   **Design:** "TRON Legacy" Ästhetik (Neon-Blau & Neon-Orange auf Schwarz, Grid-Lines).

### Features
*   **System Link:** Start-Modal zur Eingabe der IP-Adressen beider Radare.
*   **Dual Scope:**
    *   **Links (Blau):** 360° Ansicht des Ultraschall-Radars. Zeigt Live-Werte (cm) und eine dauerhafte Karte.
    *   **Rechts (Orange):** 180° Ansicht des LIDARs. Zeigt präzise Scans mit Nachleucht-Effekt.
*   **Interaktive Steuerung:**
    *   Geschwindigkeits-Slider für das Ultraschall-Radar.
    *   "Reset Map" Button zum Löschen der Karte.
*   **HUD:** Head-Up-Display mit Live-Daten, Status-Anzeigen (ONLINE/OFFLINE) und Richtungsbeschriftungen (FRONT, BACK, LEFT, RIGHT).

### Deployment
*   Das System liegt als ausführbare Datei (`TRON_Radar.html`) auf dem Desktop und kann ohne Installation gestartet werden.
