# Temperature with Time - Cheap Yellow Display (ESP32-2432S028)

A comprehensive weather station and digital clock built for the **Cheap Yellow Display (CYD)** 2.4" ESP32 module. This project displays real-time weather data and allows you to switch between a detailed weather dashboard and a large, high-visibility clock by simply touching the screen.

![Project Preview](https://github.com/Garionhk/ESP32-CYD-2.8-Weather-and-Clock/blob/main/20260312_214831_clean.jpg) *(Example CYD Hardware)*

## ✨ Features

- **Touch-Controlled Interface**: Switch between two views by touching anywhere on the screen (no automatic cycling):
  - **Main Screen**: Displays current temperature, daily high/low, humidity, and atmospheric conditions with dynamic weather icons.
  - **Just Time Screen**: A minimalist, high-contrast LED-style clock with the current date.
- **Real-Time Weather**: Fetches live data from the Open-Meteo API using your local coordinates.
- **NTP Time Sync**: Automatically synchronizes time via WiFi using Network Time Protocol.
- **Adjustable Brightness**: Configured for maximum visibility (100% brightness).
- **LED Styling**: Uses high-visibility 7-segment/LED style fonts for the clock.

## 🛠 Hardware Requirements

- **Device**: ESP32-2432S028 (Cheap Yellow Display) 2.8"
- **Internal Components used**:
  - TFT Display (ILI9341 compatible)
  - Touch Screen (XPT2046)
  - Backlight control (GPIO 21)

## 📚 Library Dependencies

This project relies on several libraries. **It is highly recommended to use the versions provided in the `libraries/` folder** of this repository to ensure compatibility.

| Library | Purpose |
| :--- | :--- |
| **TFT_eSPI** | Core display driver |
| **TFT_Touch** | Touch screen interface |
| **ArduinoJson** | Weather API data parsing |
| **Time** | Internal time management |
| **FastLED** | (Support library) |
| **TJpg_Decoder** | (Support library) |

### Library Installation
1. Locate your Arduino `libraries` folder (usually in `Documents/Arduino/libraries`).
2. Copy all folders from the `libraries/` directory of this project into your Arduino libraries folder.
3. Restart your Arduino IDE.

## ⚙️ Configuration

Open `Temperature_with_Time.ino` and update the following variables at the top of the file:

### 1. WiFi Settings
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 2. Location & Timezone
```cpp
// Set your coordinates (Example: Vancouver)
float latitude = 49.2827; 
float longitude = -123.1207;

// Timezone offset in seconds (PST is -28800)
const long gmtOffset_sec = -28800; 
const int daylightOffset_sec = 3600; // 3600 for DST, 0 for standard
```

## 🚀 How to Upload

1. Connect your CYD to your computer via USB.
2. In the Arduino IDE, go to **Tools** > **Board** and select **ESP32 Dev Module**.
3. Ensure the **TFT_eSPI** library is configured for the CYD pinout (the version in the `libraries` folder should already be pre-configured).
4. Click **Upload**.

## 📝 License
This project is licensed under the Apache License 2.0.
