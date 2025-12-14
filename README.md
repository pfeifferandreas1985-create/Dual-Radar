# NERO SENSE - Dual Radar System

**Nero Sense** is a high-tech, dual-system radar dashboard designed for environmental scanning and visualization. It combines an Ultrasonic Radar (System A) and a TOF Lidar (System B) into a unified "Tactical Dashboard" web interface.

## 🚀 Features

*   **Dual System Support**:
    *   **System A (Ultrasonic)**: Long-range, multi-sensor scanning (HC-SR04).
    *   **System B (TOF Lidar)**: High-precision, fast scanning (VL53L1X) with local display.
*   **Tactical Dashboard**: A futuristic web interface (HTML5/Canvas) featuring:
    *   **TRON & Matrix Themes**: Distinct visual styles for each system.
    *   **Real-time Visualization**: Smooth radar sweep, persistent object detection, and fading trails.
    *   **Dynamic Scaling**: Adjustable range controls (Zoom).
    *   **Telemetry**: Live distance and angle readouts.
*   **Hybrid Mode (System B)**: The TOF unit operates as a standalone device with a local **GC9A01 Circular Display**, showing a "Tactical Grid" and data even without WiFi.

---

## 🛠 Hardware & Wiring

### System B: TOF Lidar (Hybrid Unit)
*   **MCU**: Wemos D1 Mini (ESP8266)
*   **Sensor**: VL53L1X (TOF0400)
*   **Display**: 1.28" GC9A01 Circular TFT (240x240)
*   **Actuator**: Micro Servo (SG90)

#### Wiring Diagram
| Component | Pin | Wemos D1 Mini (GPIO) |
| :--- | :--- | :--- |
| **GC9A01** | **SCLK** | **D5** (GPIO 14) |
| | **MOSI** | **D7** (GPIO 13) |
| | **CS** | **D8** (GPIO 15) |
| | **DC** | **D3** (GPIO 0) |
| | **RST** | **D0** (GPIO 16) |
| | **VCC** | 3.3V |
| | **GND** | GND |
| **VL53L1X** | **SDA** | **D2** (GPIO 4) |
| | **SCL** | **D1** (GPIO 5) |
| | **VCC** | 3.3V |
| | **GND** | GND |
| **Servo** | **Signal** | **D6** (GPIO 12) |
| | **VCC** | 5V |
| | **GND** | GND |

---

## 💾 Firmware Installation

### Prerequisites
1.  **Arduino IDE**: Install the latest version.
2.  **ESP8266 Board Manager**: Install `esp8266` by ESP8266 Community.
3.  **Libraries**: Install via Library Manager:
    *   `TFT_eSPI` by Bodmer
    *   `VL53L1X` by Pololu
    *   `WebSockets` by Markus Sattler
    *   `Servo` (Built-in)

### Configuration (Critical!)
You **MUST** configure the `TFT_eSPI` library to work with the GC9A01 and Wemos D1 Mini pins.
1.  Go to your Arduino libraries folder: `Documents/Arduino/libraries/TFT_eSPI/`
2.  Edit `User_Setup.h`.
3.  Replace its content with the settings found in: `Firmware/Nereo_Sense_Hybrid_Wemos/User_Setup_Wemos.txt`

### Uploading
1.  Open `Firmware/Nereo_Sense_Hybrid_Wemos/Nero_Sense_Hybrid_Wemos.ino`.
2.  Set your WiFi SSID and Password in the code (if not already set).
3.  Select Board: **LOLIN(WEMOS) D1 R2 & mini**.
4.  Upload!

---

## 🖥️ Dashboard Usage

1.  **Open the Dashboard**: Open `Software/Dashboard/TRON_Radar.html` in any modern web browser.
2.  **Connect**:
    *   Enter the IP address of System A (Default: `192.168.178.70`).
    *   Enter the IP address of System B (Default: `192.168.178.41`).
    *   Click **CONNECT**.
3.  **Controls**:
    *   **Range**: Adjust the input field (e.g., "50" cm or "1000" mm) to zoom in/out.
    *   **Visuals**: Enjoy the show!

---

## 📂 Repository Structure

*   `Docs/`: Documentation files.
*   `Firmware/`: Arduino source code.
    *   `Nereo_Sense_Hybrid_Wemos/`: Firmware for System B (TOF).
    *   `LidarRadar/`: Firmware for System A (Ultrasonic).
*   `Hardware/`: CAD files and 3D models.
*   `Software/`: PC-side software.
    *   `Dashboard/`: The HTML5 Web Interface.

---

**Nero Sense Project** - *Advanced Environmental Awareness*
