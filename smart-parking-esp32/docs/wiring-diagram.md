# 🔌 Wiring Diagram - Smart Parking System

## Hardware Components List

| # | Component | Quantity | Notes |
|---|-----------|----------|-------|
| 1 | ESP32 DevKit V1 | 1 | Main microcontroller |
| 2 | IR Obstacle Sensor | 2 | Entry & Exit detection |
| 3 | SG90 Servo Motor | 1 | Barrier gate |
| 4 | DHT22 Sensor | 1 | Temperature & Humidity |
| 5 | LCD 16x2 I2C | 1 | Status display |
| 6 | LED (Green) | 1 | Available indicator |
| 7 | LED (Red) | 1 | Full indicator |
| 8 | Resistor 220Ω | 2 | For LEDs |
| 9 | Breadboard | 1 | For prototyping |
| 10 | Jumper Wires | ~20 | Connections |
| 11 | 5V Power Supply | 1 | For servo & LCD |

---

## Pin Connections

### ESP32 GPIO Pinout

```
                    ┌─────────────────┐
                    │     ESP32       │
                    │    DevKit V1    │
                    ├─────────────────┤
              EN ───┤ EN          D23 ├───
              VP ───┤ VP          D22 ├─── LCD SCL (I2C)
              VN ───┤ VN          TX0 ├───
         DHT22 ─────┤ D34         RX0 ├───
                    ├ D35         D21 ├─── LCD SDA (I2C)
                    ├ D32         D19 ├─── IR Exit Sensor
                    ├ D33         D18 ├─── IR Entry Sensor
    Red LED ────────┤ D25          D5 ├───
    Green LED ──────┤ D26         D17 ├───
    Servo Signal ───┤ D27         D16 ├───
                    ├ D14          D4 ├─── DHT22 Data
                    ├ D12          D2 ├───
                    ├ D13         D15 ├───
             GND ───┤ GND         GND ├─── GND
             VIN ───┤ VIN         3V3 ├─── 3.3V
                    └─────────────────┘
```

---

## Detailed Connections

### 1️⃣ IR Entry Sensor (GPIO 18)

```
IR Sensor          ESP32
─────────          ─────
  VCC  ──────────── 3.3V
  GND  ──────────── GND
  OUT  ──────────── GPIO 18
```

### 2️⃣ IR Exit Sensor (GPIO 19)

```
IR Sensor          ESP32
─────────          ─────
  VCC  ──────────── 3.3V
  GND  ──────────── GND
  OUT  ──────────── GPIO 19
```

### 3️⃣ Servo Motor (GPIO 25)

```
Servo (SG90)       ESP32
────────────       ─────
  Red (VCC)  ────── 5V (External)
  Brown (GND) ───── GND
  Orange (Signal) ─ GPIO 25
```

⚠️ **Important**: Use external 5V supply for servo, not ESP32's 5V pin!

### 4️⃣ DHT22 Sensor (GPIO 4)

```
DHT22              ESP32
─────              ─────
  VCC  ──────────── 3.3V
  GND  ──────────── GND
  DATA ──────────── GPIO 4
  
  (Add 10kΩ pull-up resistor between VCC and DATA)
```

### 5️⃣ LCD I2C Display

```
LCD I2C            ESP32
───────            ─────
  VCC  ──────────── 5V
  GND  ──────────── GND
  SDA  ──────────── GPIO 21
  SCL  ──────────── GPIO 22
```

### 6️⃣ LED Indicators

```
Green LED          ESP32
─────────          ─────
  Anode (+) ─[220Ω]─ GPIO 26
  Cathode (-) ────── GND

Red LED            ESP32
───────            ─────
  Anode (+) ─[220Ω]─ GPIO 27
  Cathode (-) ────── GND
```

---

## Complete Wiring Diagram (ASCII)

```
                                    ┌─────────────────────────────────┐
                                    │           ESP32                 │
                                    │                                 │
    ┌──────────────┐               │  GPIO 18 ◄────── IR Entry OUT   │
    │  IR Entry    │               │  GPIO 19 ◄────── IR Exit OUT    │
    │  Sensor      │───────────────│  GPIO 25 ───────► Servo Signal  │
    └──────────────┘               │  GPIO 26 ───────► Green LED (+) │
                                    │  GPIO 27 ───────► Red LED (+)   │
    ┌──────────────┐               │  GPIO 4  ◄────── DHT22 Data     │
    │  IR Exit     │               │  GPIO 21 ◄─────► LCD SDA        │
    │  Sensor      │───────────────│  GPIO 22 ◄─────► LCD SCL        │
    └──────────────┘               │                                 │
                                    │  3.3V ──────────► Sensors VCC   │
    ┌──────────────┐               │  5V   ──────────► LCD VCC       │
    │  DHT22       │               │  GND  ──────────► Common GND    │
    │              │───────────────│                                 │
    └──────────────┘               └─────────────────────────────────┘
                                    
    ┌──────────────┐                        ┌──────────────┐
    │  SG90 Servo  │                        │  LCD 16x2    │
    │              │────────────────────────│  I2C         │
    └──────────────┘                        └──────────────┘
                                    
    ┌──────┐  ┌──────┐
    │ LED  │  │ LED  │
    │ Green│  │ Red  │
    └──────┘  └──────┘
```

---

## Power Requirements

| Component | Voltage | Current |
|-----------|---------|---------|
| ESP32 | 3.3V (internal) | ~240mA |
| IR Sensors (x2) | 3.3V | ~20mA each |
| DHT22 | 3.3V | ~2.5mA |
| LCD I2C | 5V | ~20mA |
| Servo SG90 | 5V | ~500mA (peak) |
| LEDs (x2) | ~2V | ~20mA each |

**Total**: ~800mA peak

💡 **Recommendation**: Use a 5V 2A power supply for stable operation.

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| LCD not displaying | Check I2C address (try 0x27 or 0x3F) |
| Servo jittering | Add capacitor (100µF) across power |
| DHT22 NaN readings | Add 10kΩ pull-up resistor |
| IR sensors always triggered | Adjust sensitivity potentiometer |
| WiFi won't connect | Check credentials, use 2.4GHz only |

---

## Notes

1. **I2C Scanner**: Run I2C scanner sketch to find LCD address
2. **Servo Power**: Never power servo from ESP32's 5V pin directly
3. **IR Sensors**: Most IR modules have LOW output when obstacle detected
4. **DHT22**: Needs 2 seconds between readings minimum
