# 🚗 Smart Parking System with ESP32 & FreeRTOS

A real-time multitasking smart parking management system built with ESP32, FreeRTOS, and IoT technologies. Features web dashboard, Telegram bot control, environmental monitoring, and barrier gate automation.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

## 📋 Features

- **Real-Time Parking Management**: Track available slots with IR sensors
- **Automatic Barrier Control**: Servo-controlled gate with entry/exit detection
- **Web Dashboard**: Beautiful responsive UI with live updates
- **Telegram Bot**: Remote monitoring via Telegram commands
- **Environmental Monitoring**: Temperature & humidity tracking (DHT22)
- **Dual-Core Processing**: Hardware tasks on Core 0, Communication on Core 1
- **8 Concurrent FreeRTOS Tasks**: Efficient multitasking architecture

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      ESP32 Dual Core                        │
├─────────────────────────────┬───────────────────────────────┤
│         Core 0              │           Core 1              │
│     (Hardware Tasks)        │    (Communication Tasks)      │
├─────────────────────────────┼───────────────────────────────┤
│  • Sensor Task (Priority 3) │  • LCD Task (Priority 1)      │
│  • Gate Task (Priority 2)   │  • Web Server Task (Priority 1)│
│  • LED Task (Priority 1)    │  • Telegram Task (Priority 1) │
│  • DHT Task (Priority 1)    │  • WiFi Task (Priority 1)     │
└─────────────────────────────┴───────────────────────────────┘
```

## 🔧 Hardware Components

| Component | Quantity | GPIO Pin | Description |
|-----------|----------|----------|-------------|
| ESP32 DevKit | 1 | - | Main microcontroller |
| IR Sensor (Entry) | 1 | GPIO 18 | Detects incoming cars |
| IR Sensor (Exit) | 1 | GPIO 19 | Detects outgoing cars |
| Servo Motor (SG90) | 1 | GPIO 25 | Controls barrier gate |
| Green LED | 1 | GPIO 26 | Indicates available slots |
| Red LED | 1 | GPIO 27 | Indicates parking full |
| DHT22 Sensor | 1 | GPIO 4 | Temperature & humidity |
| LCD I2C (16x2) | 1 | GPIO 21, 22 | Status display |

## 📊 Wiring Diagram

```
ESP32 Connections:
─────────────────
GPIO 18  ──────── IR Entry Sensor (OUT)
GPIO 19  ──────── IR Exit Sensor (OUT)
GPIO 25  ──────── Servo Signal (Orange)
GPIO 26  ──────── Green LED (+)
GPIO 27  ──────── Red LED (+)
GPIO 4   ──────── DHT22 Data
GPIO 21  ──────── LCD SDA
GPIO 22  ──────── LCD SCL
3.3V     ──────── Sensors VCC
5V       ──────── Servo VCC, LCD VCC
GND      ──────── Common Ground
```

## 🚀 Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) or Arduino IDE
- ESP32 Board Package
- Required Libraries (see `platformio.ini`)

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/YOUR_USERNAME/smart-parking-esp32.git
   cd smart-parking-esp32
   ```

2. **Configure WiFi & Telegram**
   
   Edit `include/config.h`:
   ```cpp
   #define WIFI_SSID "your_wifi_ssid"
   #define WIFI_PASSWORD "your_wifi_password"
   #define BOT_TOKEN "your_telegram_bot_token"
   ```

3. **Upload to ESP32**
   ```bash
   # Using PlatformIO
   pio run --target upload
   
   # Or use Arduino IDE
   ```

4. **Access the Dashboard**
   - Open Serial Monitor to get the IP address
   - Navigate to `http://[ESP32_IP]` in your browser

## 📱 Telegram Commands

| Command | Description |
|---------|-------------|
| `/start` | Show available commands |
| `/status` | Get current parking status |
| `/time` | Get current date & time |
| `/temp` | Get temperature & humidity |
| `/all` | Get complete system info |

## 🖥️ Web Dashboard

The web dashboard provides real-time monitoring with:
- Available/Occupied slots counter
- Gate status indicator
- Temperature & humidity readings
- WiFi & Internet connection status
- System uptime

## 📁 Project Structure

```
smart-parking-esp32/
├── README.md           # This file
├── platformio.ini      # PlatformIO configuration
├── src/
│   └── main.cpp        # Main application code
├── include/
│   └── config.h        # Configuration settings
└── docs/
    └── wiring-diagram.md
```

## 🔄 FreeRTOS Components Used

- **Tasks**: 8 concurrent tasks with priority-based scheduling
- **Queues**: Event-driven communication (entry, exit, LCD)
- **Mutexes**: Thread-safe access to shared resources
- **Dual-Core**: Task pinning for optimal performance

## 🛠️ Dependencies

```ini
lib_deps = 
    ESP32Servo
    LiquidCrystal_I2C
    DHT sensor library
    ArduinoJson
    UniversalTelegramBot
```

## 📈 Future Improvements

- [ ] Add RFID/NFC for authorized access
- [ ] Implement mobile app
- [ ] Add camera integration for plate recognition
- [ ] Cloud logging with Firebase/AWS
- [ ] Multiple parking zones support

## 👨‍💻 Author

** Joox** - ** Ahmed **
- IoT & AI Developer @ VoltX
- CS Student @ Helwan University 

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Helwan University - Faculty of Computers and Information Technology
- FreeRTOS community
- ESP32 Arduino community

---

⭐ Star this repo if you find it useful!
