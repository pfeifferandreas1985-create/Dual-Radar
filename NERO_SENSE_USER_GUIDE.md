# NERO SENSE - SYSTEM B: OPERATIONAL GUIDE
**Advanced Capabilities & User Manual**

## 1. System Philosophy
The **Nero Sense System B** is not just a distance sensor; it is a **Tactical Environmental Awareness Unit**. Unlike simple ultrasonic sensors, it provides a high-resolution, 180-degree cross-section of the world, visualized in real-time with a "Tron-inspired" aesthetic. It bridges the gap between raw sensor data and human-readable tactical information.

## 2. Visual Interface: How to Read the Screen
The circular display is designed for instant readability:

*   **The Cyan Wall (Contours)**:
    *   This continuous blue line represents physical obstacles.
    *   **Smooth Curves**: Indicate flat walls or large objects.
    *   **Jagged Edges**: Indicate complex geometry or scattered objects.
    *   *Capability*: Allows you to instantly see the shape of a room or corridor.

*   **The Shadow Zone (Dark Navy)**:
    *   The dark lines extending from the "Wall" to the edge of the screen represent **Occluded Space**.
    *   This is the "Unknown" area behind an object.
    *   *Capability*: Helps in understanding blind spots. If a shadow moves, the object is moving.

*   **The Scanner (Red Laser)**:
    *   Shows the current active measurement angle.
    *   *Capability*: Verifies that the system is live and scanning.

*   **The Dashboard**:
    *   **Center**: Exact distance in millimeters to the object at the current angle.
    *   **Bottom**: Current Angle (0-180°).
    *   **Status Dot**: Green (Networked) / Red (Standalone).

## 3. Operational Modes

### A. Standalone Mode (Red Dot)
*   **Description**: The unit operates without WiFi.
*   **Use Case**: Handheld scanning, field deployment, or visual-only monitoring.
*   **Capability**:
    *   **Portable Scanner**: Power via USB power bank. Walk around a room to "see" in the dark.
    *   **Alignment Tool**: Use the precise mm readout to align objects or measure gaps.

### B. Networked Mode (Green Dot)
*   **Description**: The unit connects to the "Nero Network" (FRITZ!Box) and broadcasts data.
*   **Use Case**: Robotics integration, remote monitoring, home automation.
*   **Capability**:
    *   **IoT Sensor**: Can trigger events in Home Assistant (e.g., "If distance < 50cm, turn on light").
    *   **Remote Eyes**: View the radar data on a PC dashboard (via `TRON_Radar.html`).

## 4. Integration & API (For Developers)
The true power of Nero Sense lies in its **WebSocket API**. It broadcasts raw data in real-time, allowing external systems to "tap in".

**Connection Details:**
*   **IP**: `192.168.178.71`
*   **Port**: `81`
*   **Protocol**: WebSocket (WS)

**Data Format (JSON):**
```json
{
  "angle": 45,      // Current Servo Angle (0-180)
  "distance": 1250  // Distance in millimeters
}
```

### Python Integration Example
You can control a robot based on this data. Here is a snippet for a "Collision Avoidance" script:

```python
import websocket
import json

def on_message(ws, message):
    data = json.loads(message)
    angle = data['angle']
    dist = data['distance']
    
    # Simple Logic: If obstacle ahead is close, STOP.
    if 70 < angle < 110 and dist < 300:
        print(f"CRITICAL: Obstacle at {dist}mm! STOP ROBOT!")
        # robot.stop()

ws = websocket.WebSocketApp("ws://192.168.178.71:81/", on_message=on_message)
ws.run_forever()
```

## 5. Advanced Use Cases

### 1. Hexapod Navigation (SLAM)
Mount the Nero Sense on the **Nero Hexapod**.
*   **Capability**: The robot can scan its environment to build a 2D map.
*   **Logic**: Combine the Radar Angle + Robot Rotation to create a 360° map (SLAM).

### 2. Security Tripwire
Place the unit facing a doorway.
*   **Capability**: Detect intruders.
*   **Logic**: If the "Wall" suddenly changes distance (e.g., from 2000mm to 500mm), someone has walked through.

### 3. Object Profiling
Place an object on a turntable in front of the sensor.
*   **Capability**: 3D Scanning (Basic).
*   **Logic**: Record distance profiles as the object rotates to reconstruct its shape digitally.

---
*Nero Robotics - Advanced Systems Division*
