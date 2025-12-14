/*
 * NEREO SENSE HYBRID - SYSTEM B (TOF)
 * -----------------------------------
 * Hardware: ESP-12F, GC9A01 Display, TOF0400 (VL53L1X), Servo
 * Features: Hybrid Mode (WiFi/Offline), Circular Radar Viz, WebSocket
 */

#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <Wire.h>
#include <VL53L1X.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

// --- CONFIGURATION ---
// Network
const char* ssid = "FRITZ!Box 6660 Cable UE";
const char* password = "YOUR_WIFI_PASSWORD"; // USER: PLEASE SET PASSWORD
IPAddress local_IP(192, 168, 178, 41);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

// Pins
#define SERVO_PIN 12
#define SDA_PIN 4
#define SCL_PIN 5

// Radar Settings
#define MAX_DIST_MM 1000  // Max distance for visualization (1m)
#define ANGLE_MIN 0
#define ANGLE_MAX 180
#define ANGLE_STEP 2      // Degrees per step

// Colors (RGB565)
#define C_BLACK   0x0000
#define C_CYAN    0x07FF
#define C_GREEN   0x07E0
#define C_RED     0xF800
#define C_ORANGE  0xFD20
#define C_DARK    0x0022  // Very dark grey for fade

// --- OBJECTS ---
TFT_eSPI tft = TFT_eSPI();
VL53L1X sensor;
Servo myServo;
WebSocketsServer webSocket = WebSocketsServer(81);

// --- STATE ---
bool isOnline = false;
int currentAngle = 0;
int direction = 1; // 1 = up, -1 = down
unsigned long lastServoMove = 0;
int servoDelay = 15; // Speed control

// Radar Map (Simple persistence for fade effect)
// We store the distance at each angle to redraw/clear
uint16_t distMap[181]; 

void setup() {
  Serial.begin(115200);
  
  // 1. INIT DISPLAY
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(C_BLACK);
  
  // Boot Logo (Using built-in font to avoid SPIFFS error)
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_CYAN, C_BLACK);
  tft.setTextSize(2); 
  tft.drawString("NEREO SENSE", 120, 100);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, C_BLACK);
  tft.drawString("System Init...", 120, 140);
  delay(1000);

  // 2. INIT SENSORS
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  
  sensor.setTimeout(500);
  if (!sensor.init()) {
    tft.setTextColor(C_RED, C_BLACK);
    tft.drawString("SENSOR FAIL", 120, 160);
    Serial.println("Failed to detect and initialize sensor!");
    // Non-blocking error loop to prevent WDT reset
    while (1) { 
      yield(); // Feed the watchdog
      delay(100); 
    }
  }
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(15000); // 15ms
  sensor.startContinuous(15);

  myServo.attach(SERVO_PIN);

  // 3. CONNECTIVITY MANAGER
  tft.fillRect(0, 130, 240, 40, C_BLACK); // Clear text
  tft.drawString("Connecting WiFi...", 120, 140);
  
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password); // Note: Password needs to be set!

  unsigned long startAttempt = millis();
  bool connected = false;
  
  while (millis() - startAttempt < 10000) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(100);
    // Loading bar?
    int bar = map(millis() - startAttempt, 0, 10000, 0, 100);
    tft.drawRect(70, 160, 100, 4, C_CYAN);
    tft.fillRect(70, 160, bar, 4, C_CYAN);
  }

  tft.fillScreen(C_BLACK);
  tft.setTextDatum(MC_DATUM);
  
  if (connected) {
    isOnline = true;
    tft.setTextColor(C_GREEN, C_BLACK);
    tft.drawString("ONLINE", 120, 120);
    tft.drawString(WiFi.localIP().toString(), 120, 140);
    webSocket.begin();
  } else {
    isOnline = false;
    tft.setTextColor(C_ORANGE, C_BLACK);
    tft.drawString("OFFLINE MODE", 120, 120);
  }
  
  delay(2000);
  tft.fillScreen(C_BLACK);
  
  // Draw Static Radar Grid
  drawGrid();
}

void loop() {
  if (isOnline) webSocket.loop();

  // Non-blocking Servo & Read Loop
  if (millis() - lastServoMove >= servoDelay) {
    lastServoMove = millis();
    
    // 1. Move Servo
    myServo.write(currentAngle);
    
    // 2. Read Sensor
    // Note: VL53L1X continuous mode reads in background, we just get latest
    sensor.read();
    uint16_t dist = sensor.ranging_data.range_mm;
    if (sensor.ranging_data.range_status != VL53L1X::RangeValid) {
      dist = 0; // Invalid
    }
    
    // 3. Update Map
    distMap[currentAngle] = dist;

    // 4. Visualize
    drawRadarLine(currentAngle, dist);
    
    // 5. Network Broadcast
    if (isOnline) {
      String json = "{\"angle\":" + String(currentAngle) + 
                    ",\"distance\":" + String(dist) + "}";
      webSocket.broadcastTXT(json);
    }

    // 6. Advance Angle
    currentAngle += direction * ANGLE_STEP;
    if (currentAngle >= ANGLE_MAX || currentAngle <= ANGLE_MIN) {
      direction *= -1;
    }
  }
}

// --- VISUALIZATION HELPERS ---

void drawGrid() {
  // Center (120, 120)
  tft.drawCircle(120, 120, 30, 0x0040); // Dark Green/Grey
  tft.drawCircle(120, 120, 60, 0x0040);
  tft.drawCircle(120, 120, 90, 0x0040);
  tft.drawCircle(120, 120, 119, C_CYAN); // Outer Ring
  
  tft.drawLine(0, 120, 240, 120, 0x0040); // Horizontal
  tft.drawLine(120, 0, 120, 240, 0x0040); // Vertical
}

void drawRadarLine(int angle, int dist) {
  int cx = 120;
  int cy = 120;
  int r = 118;

  // 1. Clear OLD Line (Fade Effect)
  // We clear the line 'behind' the current sweep direction
  int clearAngle = angle - (direction * 10); // 10 degrees behind
  if (clearAngle < 0) clearAngle += 360; // Wrap safety (though we oscillate)
  // Actually for oscillation 0-180, just clamping is enough or ignoring
  
  // Better Fade: Redraw the "slice" from a few steps ago with BLACK
  // But we want to keep the dots? 
  // Strategy: Redraw the 'sweep' line in black, then redraw the dot?
  // Or just use a 'Scanner' line that moves, and we erase the previous scanner line position.
  
  // Erase previous scanner line
  int prevAngle = angle - (direction * ANGLE_STEP);
  float prevRad = (prevAngle - 90) * DEG_TO_RAD;
  int px = cx + cos(prevRad) * r;
  int py = cy + sin(prevRad) * r;
  tft.drawLine(cx, cy, px, py, C_BLACK); 
  
  // Re-draw the DOT at prevAngle if it exists (so the line doesn't erase dots)
  // This is a simple way to keep dots persistent until the next sweep passes them?
  // Actually, let's just let the line erase, and we only draw NEW dots. 
  // For a true radar, dots fade over time. 
  
  // Draw NEW Scanner Line
  float rad = (angle - 90) * DEG_TO_RAD;
  int x = cx + cos(rad) * r;
  int y = cy + sin(rad) * r;
  tft.drawLine(cx, cy, x, y, C_RED); // Scanner Line

  // Draw Obstacle Dot
  if (dist > 0 && dist < MAX_DIST_MM) {
    int dPx = map(dist, 0, MAX_DIST_MM, 0, r);
    int ox = cx + cos(rad) * dPx;
    int oy = cy + sin(rad) * dPx;
    
    // Draw Dot
    tft.fillCircle(ox, oy, 2, C_CYAN);
  }
  
  // FADE LOGIC:
  // To prevent screen clutter, we can clear the "sector" that is 180 degrees away?
  // Or just clear the specific angle before we write to it (which we did with the line erase)
  // But the line erase only clears the line vector.
  // We should clear any old dot at this 'angle' from the previous sweep?
  // Since we don't store X/Y of old dots easily, we can just clear the whole ray at 'angle' before drawing.
  
  // Clear Ray at current angle (Erase old dot)
  // We do this by drawing a black line at 'angle' BEFORE drawing the red scanner.
  tft.drawLine(cx, cy, x, y, C_BLACK);
  
  // Now draw Red Scanner
  tft.drawLine(cx, cy, x, y, C_RED);
  
  // Now draw New Dot
  if (dist > 0 && dist < MAX_DIST_MM) {
    int dPx = map(dist, 0, MAX_DIST_MM, 0, r);
    int ox = cx + cos(rad) * dPx;
    int oy = cy + sin(rad) * dPx;
    tft.fillCircle(ox, oy, 2, C_CYAN);
  }
}
