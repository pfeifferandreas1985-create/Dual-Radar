/*
 * HARDWARE DIAGNOSTIC TOOL
 * ------------------------
 * 1. Tests GC9A01 Display (Red/Green/Blue)
 * 2. Scans I2C Bus (SDA=D2, SCL=D1) for VL53L1X (0x29)
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>

TFT_eSPI tft = TFT_eSPI();

#define SDA_PIN 4 // D2
#define SCL_PIN 5 // D1

void setup() {
  Serial.begin(115200);
  
  // 1. Init Display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("DIAGNOSTIC", 120, 40);
  
  // 2. Init I2C
  tft.setTextSize(1);
  tft.drawString("Scanning I2C...", 120, 80);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  int nDevices = 0;
  String result = "";
  
  for(byte address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
 
    if (error == 0) {
      result += "Found: 0x";
      if (address<16) result += "0";
      result += String(address, HEX);
      result += "\n";
      nDevices++;
    }
  }
  
  if (nDevices == 0) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("NO I2C DEVICES!", 120, 120);
    tft.drawString("Check Wiring:", 120, 140);
    tft.drawString("SDA -> D2", 120, 160);
    tft.drawString("SCL -> D1", 120, 180);
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("SUCCESS!", 120, 110);
    tft.drawString(result, 120, 140);
    
    if (result.indexOf("29") != -1) {
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawString("TOF DETECTED (0x29)", 120, 180);
    } else {
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.drawString("UNKNOWN DEVICE", 120, 180);
    }
  }
}

void loop() {
  // Blink status
  delay(1000);
}
