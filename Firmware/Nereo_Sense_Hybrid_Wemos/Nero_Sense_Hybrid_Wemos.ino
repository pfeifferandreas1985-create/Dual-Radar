/*
 * NERO SENSE - SYSTEM B (FINAL FIRMWARE)
 * --------------------------------------
 * Role: High-Performance Lidar Radar Unit
 * Hardware: Wemos D1 Mini (ESP8266)
 * Display: GC9A01 (240x240 Round) via Hardware SPI
 * Sensor: VL53L1X (TOF) via I2C
 * Servo: SG90 (PWM)
 *
 * --- USER_SETUP.h CONFIGURATION (TFT_eSPI) ---
 * #define USER_SETUP_INFO "Wemos_GC9A01"
 * #define GC9A01_DRIVER
 * #define TFT_WIDTH  240
 * #define TFT_HEIGHT 240
 * #define TFT_MOSI 13 // D7
 * #define TFT_SCLK 14 // D5
 * #define TFT_CS   15 // D8
 * #define TFT_DC    0 // D3
 * #define TFT_RST  16 // D0
 * #define LOAD_GLCD
 * #define LOAD_FONT2
 * #define LOAD_FONT4
 * #define LOAD_FONT6
 * #define LOAD_GFXFF
 * #define SMOOTH_FONT
 * #define SPI_FREQUENCY  27000000
 */
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <VL53L1X.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
// --- CONFIGURATION ---
const char* ssid = "FRITZ!Box 6660 Cable UE";
const char* password = "28370714691864306613";
IPAddress local_IP(192, 168, 178, 71); // Corrected to .71
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);
#define SERVO_PIN 12 // D6
#define SDA_PIN 4    // D2
#define SCL_PIN 5    // D1
#define MAX_DIST_MM 1000
#define ANGLE_MIN 0
#define ANGLE_MAX 180
#define ANGLE_STEP 2
// Colors
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_CYAN    0x07FF
#define C_RED     0xF800
#define C_GREEN   0x07E0
#define C_NAVY    0x000F // Dark Navy for Shadows
#define C_GRID    0x18E3 // Dim Grey for Grid
// --- OBJECTS ---
TFT_eSPI tft = TFT_eSPI();
VL53L1X sensor;
Servo myServo;
WebSocketsServer webSocket = WebSocketsServer(81);
// --- STATE ---
bool isOnline = false;
int currentAngle = 0;
int direction = 1;
unsigned long lastServoMove = 0;
int servoDelay = 15;
// Lidar Memory
uint16_t distHistory[181]; // Stores last distance for 0-180 deg
// Layout
const int CX = 120;
const int CY = 120;
const int R_MAX = 118;
// --- PROTOTYPES ---
void playIntro();
void drawRadar(int angle, int dist);
void drawInterface();
void setup() {
  Serial.begin(115200);
  // 1. INIT DISPLAY
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(C_BLACK);
  // 2. CINEMATIC BOOT
  playIntro();
  // 3. INIT SENSORS - ROBUST SEQUENCE
  
  // A. Power-up stabilization
  delay(1000); 
  // B. I2C Bus Reset (Fix stuck slaves)
  // Manually toggle SCL to release SDA if slave is holding it low
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, OUTPUT);
  for(int i=0; i<9; i++) {
    digitalWrite(SCL_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(SCL_PIN, LOW);
    delayMicroseconds(10);
  }
  pinMode(SCL_PIN, INPUT_PULLUP);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); // 100kHz for stability
  // C. Retry Loop for Sensor Init
  bool sensorFound = false;
  tft.setTextDatum(MC_DATUM);
  
  for(int attempt=1; attempt<=5; attempt++) {
     tft.fillRect(0, 170, 240, 40, C_BLACK);
     tft.setTextColor(C_WHITE, C_BLACK);
     tft.drawString("Init Sensor: " + String(attempt) + "/5", 120, 180);
     
     sensor.setTimeout(500);
     if (sensor.init()) {
       sensorFound = true;
       tft.setTextColor(C_GREEN, C_BLACK);
       tft.drawString("Sensor OK!", 120, 180);
       delay(500);
       break;
     }
     
     // Failed attempt
     delay(500);
  }
  if (!sensorFound) {
    tft.setTextColor(C_RED, C_BLACK);
    tft.drawString("SENSOR ERROR!", 120, 180);
    tft.drawString("Check Wiring", 120, 200);
    while(1) { yield(); delay(100); }
  }
  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);
  sensor.startContinuous(50);
  // Servo Setup (Extended Range)
  myServo.attach(SERVO_PIN, 500, 2500);
  // 4. NETWORK MANAGER (Non-Blocking Attempt)
  tft.fillScreen(C_BLACK);
  drawInterface(); // Draw UI first
  
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);
  
  unsigned long startAttempt = millis();
  bool connected = false;
  
  // Small status indicator
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.drawString("Connecting...", 120, 130);
  while (millis() - startAttempt < 5000) { // 5s Timeout
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(50);
  }
  // Clear Status Text
  tft.fillRect(80, 130, 80, 20, C_BLACK);
  if (connected) {
    isOnline = true;
    webSocket.begin();
    tft.fillCircle(120, 135, 3, C_GREEN); // Online Dot
  } else {
    isOnline = false;
    tft.fillCircle(120, 135, 3, C_RED);   // Offline Dot
  }
}
void loop() {
  if (isOnline) webSocket.loop();
  unsigned long now = millis();
  // 1. SERVO & SENSOR LOOP
  if (now - lastServoMove >= servoDelay) {
    lastServoMove = now;
    // Move Servo
    myServo.write(currentAngle);
    // Read Sensor (Non-Blocking Check)
    if (sensor.dataReady()) {
      sensor.read(false);
      uint16_t dist = sensor.ranging_data.range_mm;
      if (sensor.ranging_data.range_status != VL53L1X::RangeValid) dist = MAX_DIST_MM; // Treat invalid as max
      
      // Update History
      distHistory[currentAngle] = dist;
      // Draw Radar
      drawRadar(currentAngle, dist);
      // Broadcast
      if (isOnline) {
        String json = "{\"angle\":" + String(currentAngle) + ",\"distance\":" + String(dist) + "}";
        webSocket.broadcastTXT(json);
      }
    }
    // Advance Angle
    currentAngle += direction * ANGLE_STEP;
    if (currentAngle >= ANGLE_MAX || currentAngle <= ANGLE_MIN) {
      direction *= -1;
    }
  }
}
// --- VISUALIZATION ENGINE ---
// Global to track the last position of the red scanner
int lastScannerAngle = 0;
void drawRadar(int angle, int dist) {
  // 1. ERASE PREVIOUS SCANNER LINE & RESTORE MAP
  if (lastScannerAngle != angle) {
      float lastRad = (180 - lastScannerAngle) * DEG_TO_RAD;
      int lastEdgeX = CX + cos(lastRad) * R_MAX;
      int lastEdgeY = CY - sin(lastRad) * R_MAX;
      
      // A. Erase Red Line (Draw Black)
      tft.drawLine(CX, CY, lastEdgeX, lastEdgeY, C_BLACK);
      
      // B. Restore Grid
      for (int r = 40; r < R_MAX; r += 40) {
        int gx = CX + cos(lastRad) * r;
        int gy = CY - sin(lastRad) * r;
        tft.drawPixel(gx, gy, C_GRID);
      }
      
      // C. Restore Map Data (Classic Dot + Shadow)
      int storedDist = distHistory[lastScannerAngle];
      if (storedDist > 0 && storedDist < MAX_DIST_MM) {
          int dPx = map(storedDist, 0, MAX_DIST_MM, 0, R_MAX);
          int ox = CX + cos(lastRad) * dPx;
          int oy = CY - sin(lastRad) * dPx;
          
          // Zone B: Contour Line (Thin Cyan)
          // Instead of a thick dot, we connect to the neighbor to form a wall.
          
          // 1. Draw Shadow first (Background)
          tft.drawLine(ox, oy, lastEdgeX, lastEdgeY, C_NAVY);
          
          // 2. Draw Contour (Foreground)
          int prevIndex = lastScannerAngle - 1;
          bool connected = false;
          
          if (prevIndex >= 0) {
             int prevDist = distHistory[prevIndex];
             // Jump Filter: Only connect if depth difference is small (< 300mm)
             // This prevents connecting foreground objects to background walls.
             if (prevDist > 0 && prevDist < MAX_DIST_MM && abs(prevDist - storedDist) < 300) {
                 float prevRad = (180 - prevIndex) * DEG_TO_RAD;
                 int pdPx = map(prevDist, 0, MAX_DIST_MM, 0, R_MAX);
                 int pox = CX + cos(prevRad) * pdPx;
                 int poy = CY - sin(prevRad) * pdPx;
                 
                 tft.drawLine(pox, poy, ox, oy, C_CYAN);
                 connected = true;
             }
          }
          
          // If not connected (start of line or jump), draw a single pixel
          if (!connected) {
             tft.drawPixel(ox, oy, C_CYAN);
          }
      }
  }
  // 2. DRAW NEW SCANNER LINE
  float rad = (180 - angle) * DEG_TO_RAD;
  int edgeX = CX + cos(rad) * R_MAX;
  int edgeY = CY - sin(rad) * R_MAX;
  
  // Draw Red Line
  tft.drawLine(CX, CY, edgeX, edgeY, C_RED);
  
  // Update Tracker
  lastScannerAngle = angle;
  
  // 3. UPDATE DASHBOARD (Text)
  updateDashboard(angle, dist);
}
void updateDashboard(int angle, int dist) {
  // Clear previous text area (optimized rects)
  tft.fillRect(60, 160, 120, 60, C_BLACK);
  
  // Distance Value (Large)
  tft.setTextDatum(MC_DATUM); 
  tft.setTextColor(C_WHITE, C_BLACK);
  
  String distStr = (dist >= MAX_DIST_MM) ? "> 1000" : String(dist) + " mm";
  tft.drawString(distStr, 120, 170, 4); 
  
  // Angle Value (Small)
  tft.setTextColor(C_CYAN, C_BLACK);
  String angleStr = "ANG: " + String(angle) + " deg";
  tft.drawString(angleStr, 120, 200, 2);
  
  // Connectivity Dot
  if (isOnline) tft.fillCircle(120, 220, 4, C_GREEN);
  else tft.fillCircle(120, 220, 4, C_RED);
}
void drawInterface() {
  // Static Grid Rings
  tft.drawCircle(CX, CY, 40, C_GRID);
  tft.drawCircle(CX, CY, 80, C_GRID);
  tft.drawCircle(CX, CY, R_MAX, C_CYAN); // Outer Rim
  
  // Crosshairs
  tft.drawLine(CX - R_MAX, CY, CX + R_MAX, CY, C_GRID);
  tft.drawLine(CX, CY, CX, CY - R_MAX, C_GRID);
}
// --- CINEMATIC BOOT ---
void playIntro() {
  // T=0: Singularity
  tft.fillScreen(C_BLACK);
  tft.drawPixel(CX, CY, C_WHITE);
  delay(500);
  // T=500-1000: Pulse
  for (int r = 0; r <= 15; r++) {
    tft.fillCircle(CX, CY, r, C_WHITE);
    delay(10);
  }
  for (int r = 15; r >= 5; r--) {
    tft.fillCircle(CX, CY, r, C_BLACK); // Hollow out
    tft.drawCircle(CX, CY, r, C_WHITE);
    delay(10);
  }
  
  // T=1000-1800: Ignition (Ring Wipe)
  for (int r = 0; r < 140; r+=4) {
    tft.drawCircle(CX, CY, r, C_CYAN);
    tft.drawCircle(CX, CY, r-1, C_CYAN);
    delay(5);
  }
  tft.fillScreen(C_BLACK);
  // T=1800-3000: Identity
  tft.setTextDatum(MC_DATUM);
  
  // Glitch Effect
  for(int i=0; i<3; i++) {
    int ox = random(-2, 3);
    int oy = random(-2, 3);
    tft.setTextColor(C_WHITE, C_BLACK);
    tft.drawString("NERO", CX+ox, CY-20+oy, 4);
    tft.setTextColor(C_CYAN, C_BLACK);
    tft.drawString("ROBOTICS", CX-ox, CY+10-oy, 2);
    delay(50);
    tft.fillScreen(C_BLACK);
  }
  
  // Final Text
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.drawString("NERO", CX, CY-20, 4);
  tft.setTextColor(C_CYAN, C_BLACK);
  tft.drawString("ROBOTICS", CX, CY+10, 2);
  
  // Lines
  tft.drawLine(60, CY-40, 180, CY-40, C_GRID);
  tft.drawLine(60, CY+30, 180, CY+30, C_GRID);
  
  delay(1000);
  // T=3000-4000: Boot
  tft.drawRect(70, 200, 100, 4, C_GRID);
  for(int i=0; i<100; i+=2) {
    tft.fillRect(72, 202, i, 2, C_GREEN);
    delay(10);
  }
  
  // Flash
  tft.fillScreen(C_WHITE);
  delay(50);
  tft.fillScreen(C_BLACK);
}
