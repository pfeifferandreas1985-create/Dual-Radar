/*
 * TRON Style LIDAR Radar System
 * Hardware: ESP-12F, VL53L1X, Micro Servo
 * 
 * Wiring:
 *  - VL53L1X SDA: GPIO 4
 *  - VL53L1X SCL: GPIO 5
 *  - Servo Signal: GPIO 14
 *  - Status LED:   GPIO 2 (Built-in, Active Low)
 *  
 * Libraries required:
 *  - VL53L1X (Pololu)
 *  - WebSockets (Markus Sattler)
 *  - Servo (Built-in)
 *  - ESP8266WiFi (Built-in)
 */

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <VL53L1X.h>
#include <Servo.h>

// ===================================================================================
// CONFIGURATION
// ===================================================================================
const char* ssid     = "ESP-RADAR";
const char* password = "password123"; // Set to NULL for open network

const int PIN_SDA   = 4;
const int PIN_SCL   = 5;
const int PIN_SERVO = 14;
const int PIN_LED   = 2;

// Servo Settings
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;
int servoDelay = 15; // ms between steps (controls sweep speed) - Dynamic

// Sensor Settings
#define SENSOR_BUDGET 20000 // us (20ms)

// ===================================================================================
// GLOBALS
// ===================================================================================
VL53L1X sensor;
Servo scanner;
WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer server(80);

int currentAngle = 90;
int angleStep = 1;
unsigned long lastServoMove = 0;
bool longRangeMode = false;

// ===================================================================================
// FRONTEND (HTML/CSS/JS)
// ===================================================================================
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>TRON LIDAR</title>
<style>
  :root {
    --bg-color: #000000;
    --grid-color: #1a1a1a;
    --primary-color: #00FFFF;
    --alert-color: #FF0000;
    --panel-bg: rgba(0, 20, 20, 0.8);
  }
  body { 
    margin: 0; 
    background: var(--bg-color); 
    overflow: hidden; 
    font-family: 'Segoe UI', 'Roboto', monospace; 
    color: var(--primary-color);
    user-select: none;
  }
  #radar-container { 
    position: relative; 
    width: 100vw; 
    height: 100vh; 
    display: flex; 
    justify-content: center; 
    align-items: center; 
  }
  canvas { 
    border-radius: 50%; 
    background: radial-gradient(circle, #001111 0%, #000000 90%); 
    box-shadow: 0 0 30px rgba(0, 255, 255, 0.1);
  }
  .hud { 
    position: absolute; 
    top: 20px; 
    left: 20px; 
    z-index: 10; 
    pointer-events: none; 
    text-shadow: 0 0 5px var(--primary-color);
  }
  .controls {
    position: absolute;
    bottom: 20px;
    left: 50%;
    transform: translateX(-50%);
    z-index: 20;
    background: var(--panel-bg);
    padding: 15px;
    border: 1px solid var(--primary-color);
    border-radius: 10px;
    display: flex;
    gap: 20px;
    align-items: center;
    box-shadow: 0 0 15px rgba(0, 255, 255, 0.2);
    backdrop-filter: blur(5px);
  }
  h1 { margin: 0; font-size: 20px; letter-spacing: 2px; border-bottom: 1px solid var(--primary-color); display: inline-block; padding-bottom: 5px; }
  .data-row { margin-top: 10px; font-size: 14px; font-family: 'Courier New', monospace; }
  .label { opacity: 0.7; margin-right: 10px; }
  .value { font-weight: bold; font-size: 16px; }
  #status { font-weight: bold; color: var(--alert-color); }
  #status.connected { color: #00FF00; text-shadow: 0 0 5px #00FF00; }
  
  /* Inputs */
  button {
    background: transparent;
    border: 1px solid var(--primary-color);
    color: var(--primary-color);
    padding: 8px 16px;
    font-family: inherit;
    font-weight: bold;
    cursor: pointer;
    text-transform: uppercase;
    transition: all 0.2s;
  }
  button:hover { background: rgba(0, 255, 255, 0.2); box-shadow: 0 0 10px var(--primary-color); }
  button:active { transform: scale(0.95); }
  
  input[type=range] {
    -webkit-appearance: none;
    background: transparent;
    width: 150px;
  }
  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    height: 16px; width: 16px;
    border-radius: 50%;
    background: var(--primary-color);
    cursor: pointer;
    margin-top: -6px;
    box-shadow: 0 0 10px var(--primary-color);
  }
  input[type=range]::-webkit-slider-runnable-track {
    width: 100%; height: 4px;
    background: #333;
    border-radius: 2px;
  }
  
  .control-group { display: flex; flex-direction: column; align-items: center; }
  .control-label { font-size: 10px; margin-bottom: 5px; opacity: 0.8; }

  /* Scan line effect overlay */
  .scan-overlay {
    position: absolute;
    top: 0; left: 0; right: 0; bottom: 0;
    background: linear-gradient(to bottom, rgba(0,255,255,0), rgba(0,255,255,0.05) 50%, rgba(0,255,255,0));
    background-size: 100% 4px;
    pointer-events: none;
    z-index: 5;
  }
</style>
</head>
<body>
<div class="hud">
  <h1>TRON_LIDAR_SYSTEM</h1>
  <div class="data-row"><span class="label">LINK:</span><span id="status">OFFLINE</span></div>
  <div class="data-row"><span class="label">AZIMUTH:</span><span id="angle" class="value">0</span>&deg;</div>
  <div class="data-row"><span class="label">RANGE:</span><span id="dist" class="value">0</span> mm</div>
  <div class="data-row"><span class="label">MODE:</span><span id="modeDisplay" class="value">SHORT</span></div>
</div>

<div class="controls">
  <div class="control-group">
    <span class="control-label">SCAN SPEED</span>
    <!-- Reverse logic: Lower delay = Higher speed. Slider: 5 (Fast) to 100 (Slow) -->
    <!-- Let's make the slider intuitive: Left(Slow) -> Right(Fast). 
         Value 0-100. Map to Delay 100-5. -->
    <input type="range" id="speedSlider" min="0" max="100" value="80">
  </div>
  <div class="control-group">
    <span class="control-label">SENSOR RANGE</span>
    <button id="rangeBtn" onclick="toggleRange()">SHORT (1.3m)</button>
  </div>
</div>

<div class="scan-overlay"></div>
<div id="radar-container">
  <canvas id="radar"></canvas>
</div>
<script>
  const canvas = document.getElementById('radar');
  const ctx = canvas.getContext('2d');
  const statusEl = document.getElementById('status');
  const angleEl = document.getElementById('angle');
  const distEl = document.getElementById('dist');
  const modeDisplay = document.getElementById('modeDisplay');
  const rangeBtn = document.getElementById('rangeBtn');
  const speedSlider = document.getElementById('speedSlider');

  let width, height, centerX, centerY, maxRadius;
  const points = []; 
  let maxDist = 1500; // Default Short
  let ws;

  function resize() {
    width = window.innerWidth;
    height = window.innerHeight;
    canvas.width = width;
    canvas.height = height;
    centerX = width / 2;
    centerY = height / 2;
    maxRadius = Math.min(width, height) / 2 - 40;
  }
  window.addEventListener('resize', resize);
  resize();

  // WebSocket Connection
  function connect() {
    ws = new WebSocket('ws://' + location.hostname + ':81/');
    ws.onopen = () => { 
      statusEl.textContent = "ESTABLISHED"; 
      statusEl.classList.add('connected'); 
    };
    ws.onclose = () => { 
      statusEl.textContent = "LOST - RETRYING"; 
      statusEl.classList.remove('connected'); 
      setTimeout(connect, 2000);
    };
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        angleEl.textContent = data.angle;
        distEl.textContent = data.distance;
        
        points.push({
          angle: data.angle,
          dist: data.distance,
          time: Date.now()
        });
      } catch(e) {}
    };
  }
  connect();

  // Controls
  let isLongRange = false;
  function toggleRange() {
    isLongRange = !isLongRange;
    if (isLongRange) {
      ws.send('R1');
      rangeBtn.textContent = "LONG (4.0m)";
      modeDisplay.textContent = "LONG";
      maxDist = 4000;
    } else {
      ws.send('R0');
      rangeBtn.textContent = "SHORT (1.3m)";
      modeDisplay.textContent = "SHORT";
      maxDist = 1500;
    }
    // Clear old points to avoid confusion
    points.length = 0;
  }

  speedSlider.oninput = function() {
    // Map 0-100 to 100ms-5ms
    // 0 -> 100
    // 100 -> 5
    const val = parseInt(this.value);
    const delay = Math.floor(100 - (val * 0.95)); // Approx mapping
    if(ws && ws.readyState === WebSocket.OPEN) {
      ws.send('S' + delay);
    }
  };

  function drawGrid() {
    ctx.strokeStyle = '#1a1a1a';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let r = 0.25; r <= 1; r += 0.25) {
      ctx.arc(centerX, centerY, maxRadius * r, 0, Math.PI * 2);
    }
    ctx.stroke();
    
    ctx.beginPath();
    ctx.moveTo(centerX - maxRadius, centerY);
    ctx.lineTo(centerX + maxRadius, centerY);
    ctx.moveTo(centerX, centerY - maxRadius);
    ctx.lineTo(centerX, centerY + maxRadius);
    ctx.stroke();
  }

  function draw() {
    ctx.fillStyle = 'rgba(0, 0, 0, 0.08)';
    ctx.fillRect(0, 0, width, height);

    drawGrid();

    const now = Date.now();
    
    for (let i = points.length - 1; i >= 0; i--) {
      const p = points[i];
      const age = now - p.time;
      if (age > 3000) {
        points.splice(i, 1);
        continue;
      }

      const rad = -(p.angle * Math.PI / 180);
      const r = (p.dist / maxDist) * maxRadius;
      const drawR = Math.min(r, maxRadius);

      const x = centerX + Math.cos(rad) * drawR;
      const y = centerY + Math.sin(rad) * drawR;

      const alpha = 1 - (age / 3000);
      
      ctx.beginPath();
      ctx.arc(x, y, 4, 0, Math.PI * 2);
      ctx.fillStyle = `rgba(0, 255, 255, ${alpha})`;
      
      if (age < 200) {
        ctx.shadowBlur = 15;
        ctx.shadowColor = '#00FFFF';
      } else {
        ctx.shadowBlur = 0;
      }
      
      ctx.fill();
      ctx.shadowBlur = 0;
    }
    
    requestAnimationFrame(draw);
  }
  draw();
</script>
</body>
</html>
)=====";

// ===================================================================================
// WEBSOCKET EVENT HANDLER
// ===================================================================================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    // Command Parsing
    if (payload[0] == 'S') {
      // Speed Command: S<delay_ms>
      int val = atoi((const char*)&payload[1]);
      if (val < 5) val = 5;
      if (val > 200) val = 200;
      servoDelay = val;
    }
    else if (payload[0] == 'R') {
      // Range Command: R0 (Short) or R1 (Long)
      int val = atoi((const char*)&payload[1]);
      bool newMode = (val == 1);
      
      if (newMode != longRangeMode) {
        longRangeMode = newMode;
        
        // Reconfigure Sensor
        sensor.stopContinuous();
        delay(10); // Brief pause
        
        if (longRangeMode) {
          sensor.setDistanceMode(VL53L1X::Long);
          // Long mode needs more time budget
          sensor.setMeasurementTimingBudget(33000); 
        } else {
          sensor.setDistanceMode(VL53L1X::Short);
          sensor.setMeasurementTimingBudget(20000);
        }
        
        sensor.startContinuous(longRangeMode ? 33 : 20);
      }
    }
  }
}

// ===================================================================================
// SETUP
// ===================================================================================
void setup() {
  // 1. Init Pins
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH); // LED Off

  // 2. Init I2C
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000); // 400kHz I2C

  // 3. Init Sensor
  sensor.setTimeout(500);
  if (!sensor.init()) {
    // FAILURE: Rapid Blink
    while (1) {
      digitalWrite(PIN_LED, LOW);  delay(50);
      digitalWrite(PIN_LED, HIGH); delay(50);
    }
  }
  
  // Default: Short Mode
  sensor.setDistanceMode(VL53L1X::Short);
  sensor.setMeasurementTimingBudget(SENSOR_BUDGET);
  sensor.startContinuous(SENSOR_BUDGET / 1000);

  // 4. Init Servo
  scanner.attach(PIN_SERVO);
  scanner.write(currentAngle);

  // 5. Init WiFi (AP Mode)
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // 6. Init Servers
  server.on("/", []() {
    server.send_P(200, "text/html", index_html);
  });
  server.begin();
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// ===================================================================================
// LOOP
// ===================================================================================
void loop() {
  webSocket.loop();
  server.handleClient();

  unsigned long now = millis();

  // Non-blocking Servo Sweep
  if (now - lastServoMove >= servoDelay) {
    lastServoMove = now;
    
    // Update Angle
    currentAngle += angleStep;
    if (currentAngle >= SERVO_MAX_ANGLE || currentAngle <= SERVO_MIN_ANGLE) {
      angleStep = -angleStep; // Reverse direction
    }
    scanner.write(currentAngle);
    
    if (sensor.dataReady()) {
      uint16_t dist = sensor.read(false); // Non-blocking read
      
      // Broadcast JSON
      String json = "{\"angle\":";
      json += currentAngle;
      json += ",\"distance\":";
      json += dist;
      json += "}";
      webSocket.broadcastTXT(json);
    }
  }
}
