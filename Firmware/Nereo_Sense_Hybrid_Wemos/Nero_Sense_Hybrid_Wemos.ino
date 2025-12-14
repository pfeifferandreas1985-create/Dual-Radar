/*
 * NERO SENSE HYBRID - SYSTEM B (TOF)
 * ----------------------------------
 * Hardware: Wemos D1 Mini (ESP8266), GC9A01 Display, TOF0400 (VL53L1X), Servo
 * Features: Tactical Dashboard (Split Screen), Hybrid Mode, WebSocket
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
const char* password = "28370714691864306613"; 
IPAddress local_IP(192, 168, 178, 41);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

// Pins (Wemos D1 Mini)
#define SERVO_PIN 12 // D6
#define SDA_PIN 4    // D2
#define SCL_PIN 5    // D1

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
#define C_GRID    0x39E7  // Dark Grey
#define C_WHITE   0xFFFF

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

// Radar Map (Persistence)
uint16_t distMap[181]; 

// Layout Constants
const int CX = 120;
const int CY = 120; // Center of the screen, but radar origin is also here?
// User asked for "Radar Area to Top Semicircle (Y: 0 to 120)".
// This implies the center of the radar arc is at (120, 120).
const int R_MAX = 118; 

void setup() {
  Serial.begin(115200);
  
  // 1. INIT DISPLAY
  tft.init();
  tft.setRotation(0); 
  tft.fillScreen(C_BLACK);
  
  // Boot Animation
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_CYAN, C_BLACK);
  tft.setTextSize(2); 
  tft.drawString("NERO SENSE", 120, 100);
  
  tft.setTextSize(1);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.drawString("System Init...", 120, 140);
  delay(2000); 

  // 2. INIT SENSORS
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  
  sensor.setTimeout(500);
  if (!sensor.init()) {
    tft.setTextColor(C_RED, C_BLACK);
    tft.drawString("SENSOR FAIL", 120, 160);
    // Non-blocking error loop
    while (1) { yield(); delay(100); }
  }
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(15000); 
  sensor.startContinuous(15);

  myServo.attach(SERVO_PIN);

  // 3. CONNECTIVITY MANAGER
  tft.fillRect(0, 130, 240, 40, C_BLACK); 
  tft.drawString("Connecting WiFi...", 120, 140);
  
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password); 

  unsigned long startAttempt = millis();
  bool connected = false;
  
  while (millis() - startAttempt < 8000) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(100);
    int bar = map(millis() - startAttempt, 0, 8000, 0, 100);
    tft.drawRect(70, 160, 100, 4, C_CYAN);
    tft.fillRect(70, 160, bar, 4, C_CYAN);
  }

  tft.fillScreen(C_BLACK);
  
  if (connected) {
    isOnline = true;
    webSocket.begin();
  } else {
    isOnline = false;
    // Just flash offline briefly
    tft.setTextColor(C_ORANGE, C_BLACK);
    tft.drawString("OFFLINE MODE", 120, 120);
    delay(1000);
    tft.fillScreen(C_BLACK);
  }
  
  // DRAW STATIC INTERFACE
  drawInterface();
}

void loop() {
  if (isOnline) webSocket.loop();

  unsigned long now = millis();

  // 1. NON-BLOCKING SERVO
  if (now - lastServoMove >= servoDelay) {
    lastServoMove = now;
    
    // Move Servo
    myServo.write(currentAngle);
    
    // Advance Angle
    currentAngle += direction * ANGLE_STEP;
    if (currentAngle >= ANGLE_MAX || currentAngle <= ANGLE_MIN) {
      direction *= -1;
    }
  }

  // 2. NON-BLOCKING SENSOR READ
  if (sensor.dataReady()) {
    sensor.read(false); 
    uint16_t dist = sensor.ranging_data.range_mm;
    
    if (sensor.ranging_data.range_status != VL53L1X::RangeValid) {
       // dist = 0; 
    }
    
    // Update Map
    distMap[currentAngle] = dist;
    
    // VISUALIZE
    drawRadarUpdate(currentAngle, dist);
    updateDashboard(currentAngle, dist);
    
    // BROADCAST
    if (isOnline) {
      String json = "{\"angle\":" + String(currentAngle) + 
                    ",\"distance\":" + String(dist) + "}";
      webSocket.broadcastTXT(json);
    }
  }
}

// --- VISUALIZATION ---

void drawInterface() {
  // 1. Tactical Grid (Top Semicircle)
  // Center is (120, 120). We draw arcs from 180 to 360 degrees (Top half)
  // Actually, standard math: 0 is right, -90 is top, 180 is left.
  // Servo 0-180 maps to Screen 180 (Left) -> 270 (Top) -> 360/0 (Right)?
  // Let's assume Servo 0 = Left (180 deg), Servo 90 = Top (270 deg), Servo 180 = Right (0 deg).
  // Or simpler: Servo 0 = Left, Servo 180 = Right.
  
  tft.drawCircle(CX, CY, 30, C_GRID);
  tft.drawCircle(CX, CY, 60, C_GRID);
  tft.drawCircle(CX, CY, 90, C_GRID);
  tft.drawCircle(CX, CY, R_MAX, C_CYAN); // Outer Rim
  
  // Radial Lines
  // 0 deg (Right), 180 deg (Left), 90 deg (Top)
  tft.drawLine(CX - R_MAX, CY, CX + R_MAX, CY, C_GRID); // Horizontal Base
  tft.drawLine(CX, CY, CX, CY - R_MAX, C_GRID);         // Vertical Center
  
  // 45 and 135
  int r45 = R_MAX;
  int x45 = CX + cos(-45 * DEG_TO_RAD) * r45;
  int y45 = CY + sin(-45 * DEG_TO_RAD) * r45;
  tft.drawLine(CX, CY, x45, y45, C_GRID);
  
  int x135 = CX + cos(-135 * DEG_TO_RAD) * r45;
  int y135 = CY + sin(-135 * DEG_TO_RAD) * r45;
  tft.drawLine(CX, CY, x135, y135, C_GRID);

  // 2. Dashboard Separator
  tft.drawLine(0, 122, 240, 122, C_CYAN);

  // 3. Static Text Labels
  tft.setTextDatum(TC_DATUM); // Top Center
  tft.setTextColor(C_GRID, C_BLACK);
  tft.drawString("DISTANCE", 120, 135, 2);
}

void drawRadarUpdate(int angle, int dist) {
  // Map Servo Angle (0-180) to Screen Rads
  // 0 -> 180 deg (PI) -> Left
  // 180 -> 0 deg (0) -> Right
  // Formula: rad = (180 - angle) * DEG_TO_RAD

  float rad = (180 - angle) * DEG_TO_RAD;

  // 1. ERASE PREVIOUS LINE
  // We calculate where the line was
  int prevAngle = angle - (direction * ANGLE_STEP);
  float prevRad = (180 - prevAngle) * DEG_TO_RAD;

  int px = CX + cos(prevRad) * R_MAX;
  int py = CY - sin(prevRad) * R_MAX; // Subtract sin because Y is inverted

  tft.drawLine(CX, CY, px, py, C_BLACK);

  // REDRAW GRID
  if (abs(prevAngle - 90) < 2) tft.drawLine(CX, CY, CX, CY - R_MAX, C_GRID);

  // NOTE: We do NOT restore the old dot here.
  // This ensures the "wiper" clears the screen, preventing clutter.

  // 2. DRAW NEW SCANNER (RED)
  int x = CX + cos(rad) * R_MAX;
  int y = CY - sin(rad) * R_MAX;
  tft.drawLine(CX, CY, x, y, C_RED);

  // 3. DRAW OBSTACLE
  if (dist > 0 && dist < MAX_DIST_MM) {
    int dPx = map(dist, 0, MAX_DIST_MM, 0, R_MAX);
    int ox = CX + cos(rad) * dPx;
    int oy = CY - sin(rad) * dPx;
    
    tft.fillCircle(ox, oy, 3, C_CYAN);
  }
}

void updateDashboard(int angle, int dist) {
  // Bottom Half Dashboard
  // Distance Value (Large)
  tft.setTextDatum(MC_DATUM); // Middle Center
  tft.setTextColor(C_WHITE, C_BLACK); // Overwrite background
  
  // Font 6 is large numeric, Font 4 is medium
  // Using Font 4 for safety as 6 might be too big for 240px width with "mm"
  
  String distStr = String(dist) + " mm";
  tft.drawString(distStr, 120, 170, 4); 
  
  // Angle Value (Small)
  tft.setTextColor(C_CYAN, C_BLACK);
  String angleStr = "ANG: " + String(angle) + " deg";
  tft.drawString(angleStr, 120, 210, 2);
  
  // Connectivity Dot
  if (isOnline) tft.fillCircle(220, 220, 4, C_GREEN);
  else tft.fillCircle(220, 220, 4, C_RED);
}
