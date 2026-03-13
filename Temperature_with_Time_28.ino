#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <TFT_Touch.h>
#include "time.h"

// --- Configuration ---
const char* ssid = "YourWIFI";
const char* password = "YourWIFIPassword";
const char* ntpServer = "pool.ntp.org";

// Vancouver/Richmond Timezone: PST (UTC-8) / PDT (UTC-7)
const long  gmtOffset_sec = -28800; 
const int   daylightOffset_sec = 3600;

// LDR Pin for CYD
#define LDR_PIN 34
#define BACKLIGHT_PIN 21

// Touch pins for CYD 2.4 (Corrected)
#define TOUCH_CS 33
#define TOUCH_CLK 25
#define TOUCH_DIN 32
#define TOUCH_DOUT 39

// Type your location coordinates
float latitude = 49.2827; 
float longitude = -123.1207;

unsigned long lastWeatherUpdate = 0;
const long weatherInterval = 60000; 
bool colonVisible = true;
unsigned long lastTick = 0;
bool showJustTime = false;
unsigned long lastTouchTime = 0;

TFT_eSPI tft = TFT_eSPI();
TFT_Touch touch(TOUCH_CS, TOUCH_CLK, TOUCH_DIN, TOUCH_DOUT);

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  tft.fillScreen(TFT_BLACK);

  // Setup Backlight PWM
  pinMode(BACKLIGHT_PIN, OUTPUT);
  ledcSetup(0, 5000, 8); // Channel 0, 5kHz, 8-bit
  ledcAttachPin(BACKLIGHT_PIN, 0);

  // Setup Touch
  // TFT_Touch uses setCal() for calibration
  touch.setCal(200, 3700, 300, 3800, 320, 240, 1); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  updateWeather();
}

void loop() {
  if (touch.Pressed()) {
    Serial.println("Touch detected!");
    // Check if 1 second debounce has passed
    if (millis() - lastTouchTime > 1000) { 
      showJustTime = !showJustTime;
      Serial.print("Switching to screen: ");
      Serial.println(showJustTime ? "Just Time" : "Main");
      tft.fillScreen(TFT_BLACK);
      lastTouchTime = millis();
      
      // Force immediate update
      if (!showJustTime) {
        updateWeather();
      }
    }
  }

  if (millis() - lastTick >= 1000) {
    if (showJustTime) {
      displayJustTime();
    } else {
      displayClock();
      adjustBrightness();
    }
    colonVisible = !colonVisible;
    lastTick = millis();
  }

  if (!showJustTime && (millis() - lastWeatherUpdate >= weatherInterval)) {
    updateWeather();
    lastWeatherUpdate = millis();
  }
}

void displayClock() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  char timeStr[6]; // HH:MM
  strftime(timeStr, 6, "%H:%M", &timeinfo);

  // Bottom area for clock: y from 160 to 239
  // Clear only this panel area
  tft.fillRect(0, 160, 320, 80, TFT_BLACK);
  tft.drawFastHLine(0, 160, 320, TFT_DARKGREY);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  
  // Use Font 7 for LED/7-segment look
  // Middle of bottom section (approx): y=175
  if (colonVisible) {
    tft.drawCentreString(timeStr, 160, 175, 7);
  } else {
    // Hide colon by replacing it with a space in the string
    timeStr[2] = ' ';
    tft.drawCentreString(timeStr, 160, 175, 7);
  }
}

void adjustBrightness() {
  // Fixed at 100% brightness as requested
  ledcWrite(0, 255); 
}

void displayJustTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  char timeStr[6]; // HH:MM
  strftime(timeStr, 6, "%H:%M", &timeinfo);

  if (!colonVisible) {
    timeStr[2] = ' ';
  }

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  
  // Time: Double the size of Font 7
  tft.setTextSize(2);
  tft.drawCentreString(timeStr, 160, 40, 7); 
  
  // Date: Font 7 (yyyy-mm-dd) - adjusted to 90% width (288px)
  char dateStr[11]; // yyyy-mm-dd
  strftime(dateStr, 11, "%Y-%m-%d", &timeinfo);
  
  tft.setTextSize(1);
  // Target width 288px. Start at x=16. 
  // 10 characters. 288 / 10 = 28.8 pixels per character space.
  int startX = 16;
  int yPos = 160;
  
  // Clear the date area first to prevent overlapping artifacts
  tft.fillRect(0, yPos, 320, 50, TFT_BLACK);
  
  for (int i = 0; i < 10; i++) {
    int x = startX + (int)(i * 28.8);
    tft.drawChar(dateStr[i], x, yPos, 7);
  }
}

void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(latitude) + 
                 "&longitude=" + String(longitude) + 
                 "&current=temperature_2m,relative_humidity_2m,weather_code" +
                 "&daily=temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=1";

    http.begin(url);
    if (http.GET() > 0) {
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, http.getString());
      drawUI(doc["current"]["temperature_2m"], doc["daily"]["temperature_2m_max"][0], 
             doc["daily"]["temperature_2m_min"][0], doc["current"]["relative_humidity_2m"], 
             doc["current"]["weather_code"]);
    }
    http.end();
  }
}

void drawWeatherIcon(int x, int y, int code) {
  switch (code) {
    case 0: // Clear Sky - Sun
      tft.fillCircle(x, y, 15, TFT_YELLOW);
      for (int i = 0; i < 360; i += 45) {
        float rad = i * 3.14159265 / 180.0;
        int x1 = x + (int)(cos(rad) * 18);
        int y1 = y + (int)(sin(rad) * 18);
        int x2 = x + (int)(cos(rad) * 25);
        int y2 = y + (int)(sin(rad) * 25);
        tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
      }
      break;
    case 1: case 2: case 3: // Partly Cloudy
      // Sun
      tft.fillCircle(x + 10, y - 10, 10, TFT_YELLOW);
      // Cloud
      tft.fillCircle(x - 10, y + 5, 12, TFT_WHITE);
      tft.fillCircle(x + 5, y + 5, 12, TFT_WHITE);
      tft.fillCircle(x, y - 5, 15, TFT_WHITE);
      break;
    case 45: case 48: // Foggy
      for (int i = -15; i <= 15; i += 10) {
        tft.drawFastHLine(x - 20, y + i, 40, TFT_LIGHTGREY);
      }
      break;
    case 51: case 53: case 55: // Drizzle
    case 61: case 63: case 65: // Rain
    case 80: case 81: case 82: // Rain Showers
      // Cloud
      tft.fillCircle(x - 10, y - 5, 12, TFT_WHITE);
      tft.fillCircle(x + 5, y - 5, 12, TFT_WHITE);
      tft.fillCircle(x, y - 15, 15, TFT_WHITE);
      // Rain drops
      for (int i = 0; i < 3; i++) {
        tft.drawLine(x - 10 + i * 10, y + 5, x - 15 + i * 10, y + 20, TFT_BLUE);
      }
      break;
    case 71: case 73: case 75: // Snow
    case 77: // Snow Grains
    case 85: case 86: // Snow Showers
      // Cloud
      tft.fillCircle(x - 10, y - 5, 12, TFT_WHITE);
      tft.fillCircle(x + 5, y - 5, 12, TFT_WHITE);
      tft.fillCircle(x, y - 15, 15, TFT_WHITE);
      // Snow (asterisks)
      for (int i = 0; i < 3; i++) {
        tft.drawCircle(x - 10 + i * 10, y + 10, 2, TFT_WHITE);
      }
      break;
    case 95: case 96: case 99: // Thunderstorm
      // Cloud
      tft.fillCircle(x - 10, y - 5, 12, TFT_WHITE);
      tft.fillCircle(x + 5, y - 5, 12, TFT_WHITE);
      tft.fillCircle(x, y - 15, 15, TFT_WHITE);
      // Lightning
      tft.drawLine(x, y + 5, x - 8, y + 15, TFT_YELLOW);
      tft.drawLine(x - 8, y + 15, x + 2, y + 15, TFT_YELLOW);
      tft.drawLine(x + 2, y + 15, x - 6, y + 30, TFT_YELLOW);
      break;
    default:
      tft.drawCentreString("?", x, y - 10, 4);
      break;
  }
}

void drawUI(float current, float high, float low, int hum, int code) {
  // Clear weather area (0,0 to 320,159)
  tft.fillRect(0, 0, 320, 160, TFT_BLACK);
  
  // Layout dividers for top section
  tft.drawFastVLine(160, 0, 160, TFT_DARKGREY);
  tft.drawFastHLine(160, 80, 160, TFT_DARKGREY);

  // Left Panel: Icon & Condition Text
  drawWeatherIcon(80, 50, code); // Centered in left panel
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString(getWMOString(code), 80, 110, 1);

  // Right Top: Current Temp & High/Low
  tft.setTextDatum(TC_DATUM); // Top Center
  tft.setFreeFont(&FreeSansBold18pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.drawString(String(current, 1) + "C", 240, 10);
  
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_RED);
  tft.drawString("H:" + String(high, 0) + "C", 205, 50);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("L:" + String(low, 0) + "C", 275, 50);

  // Right Bottom: Humidity
  tft.setTextColor(TFT_LIGHTGREY);
  tft.drawString("Humidity", 240, 95);
  tft.setTextColor(TFT_BLUE);
  tft.setFreeFont(&FreeSansBold18pt7b);
  tft.drawString(String(hum) + "%", 240, 120);
  
  tft.setTextDatum(TL_DATUM); // Reset to default Top Left
}

String getWMOString(int code) {
  switch (code) {
    case 0: return "Clear Sky";
    case 1: case 2: case 3: return "Partly Cloudy";
    case 45: case 48: return "Foggy";
    case 51: case 53: case 55: return "Drizzle";
    case 61: case 63: case 65: return "Rain";
    case 66: case 67: return "Freezing Rain";
    case 71: case 73: case 75: return "Snow Fall";
    case 77: return "Snow Grains";
    case 80: case 81: case 82: return "Rain Showers";
    case 85: case 86: return "Snow Showers";
    case 95: return "Thunderstorm";
    case 96: case 99: return "TS with Hail";
    default: return "Unknown";
  }
}