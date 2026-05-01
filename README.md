# ESP32 Web Control Panel

## Overview

This project implements a lightweight web-based control panel for an ESP32 device. It allows you to control the built-in LED and monitor system metrics (CPU temperature, Wi-Fi signal strength, uptime) directly from a browser—without requiring additional hardware.

The system uses an asynchronous web server architecture for efficient, non-blocking performance.

---

## Features

* Remote control of onboard LED
* Real-time CPU temperature monitoring
* Wi-Fi signal strength (RSSI) display
* Device uptime tracking
* Auto-refreshing dashboard (1-second interval)
* Clean and responsive web interface
* No external components required

---

## Technologies Used

* ESP32 (Arduino framework)
* AsyncTCP
* ESPAsyncWebServer
* Embedded HTML/CSS/JavaScript (served from flash memory)

---

## Hardware Requirements

* ESP32 development board

---

## Software Requirements

* Arduino IDE (or PlatformIO)
* ESP32 board package installed
* Required libraries:

  * `WiFi.h`
  * `AsyncTCP.h`
  * `ESPAsyncWebServer.h`

---

## Setup Instructions

### 1. Configure Wi-Fi Credentials

Update the following lines in the code:

```cpp
const char* ssid = "YOUR SSID";
const char* password = "YOUR PASSWORD";
```

### 2. Upload Code

* Connect your ESP32 via USB
* Select correct board and port
* Compile and upload the code

### 3. Open Serial Monitor

* Baud rate: `115200`
* Wait for connection confirmation
* Note the printed IP address

### 4. Access Web Panel

Open a browser and navigate to:

```
http://<ESP32_IP_ADDRESS>
```

---

## API Endpoints

### `GET /`

Returns the main web interface.

### `GET /data`

Returns JSON data:

```json
{
  "led": true,
  "temp": 36.5,
  "rssi": -60,
  "uptime": "0g 01s 23d 45sn"
}
```

### `GET /toggle`

Toggles LED state.

---

## Code Structure

### Core Components

* **WiFi Connection مدیریت**: Establishes network connectivity
* **Async Web Server**: Handles HTTP requests without blocking
* **LED Control Logic**: Maintains and toggles LED state
* **System Monitoring**:

  * `temperatureRead()` for CPU temperature
  * `WiFi.RSSI()` for signal strength
  * `millis()` for uptime tracking

### Web Interface

* Embedded in `PROGMEM` to optimize RAM usage
* Uses `fetch()` API for asynchronous updates
* Dynamic DOM updates without page reload

---

## Notes

* The built-in LED is typically connected to GPIO 2 (may vary by board).
* `temperatureRead()` is specific to ESP32 internal sensor and may not be highly accurate.
* Ensure both your computer and ESP32 are on the same network.

---

## Possible Improvements

* Add authentication (basic auth or token-based)
* Implement OTA firmware updates
* Expand GPIO control (multiple pins)
* Add WebSocket support for real-time updates
* Integrate sensor modules (DHT11, BMP280, etc.)

---

## License

This project is open-source and can be modified or extended as needed.

---

## Author

Developed for embedded systems experimentation and lightweight IoT control scenarios.
