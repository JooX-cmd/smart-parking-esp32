/**
 * @file config.h
 * @brief Configuration file for Smart Parking System
 * 
 * IMPORTANT: Copy this file to config_local.h and update with your credentials
 * Do NOT commit config_local.h to version control!
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WiFi Configuration
// ============================================================================
#define WIFI_SSID "your_wifi_ssid"        // Change this to your WiFi SSID
#define WIFI_PASSWORD "your_wifi_password" // Change this to your WiFi password

// ============================================================================
// Telegram Bot Configuration
// ============================================================================
// Create a bot using @BotFather on Telegram to get your token
#define BOT_TOKEN "your_telegram_bot_token"

// ============================================================================
// Hardware Pin Configuration
// ============================================================================
// IR Sensors
#define IR_ENTRY_PIN 18    // Entry sensor GPIO
#define IR_EXIT_PIN 19     // Exit sensor GPIO

// Servo Motor (Barrier Gate)
#define SERVO_PIN 25       // Servo control GPIO

// LED Indicators
#define GREEN_LED_PIN 26   // Available slots indicator
#define RED_LED_PIN 27     // Parking full indicator

// DHT22 Sensor
#define DHT_PIN 4          // DHT22 data pin
#define DHT_TYPE DHT22     // Sensor type

// LCD I2C
#define LCD_SDA 21         // I2C SDA
#define LCD_SCL 22         // I2C SCL
#define LCD_ADDRESS 0x27   // I2C address (try 0x3F if 0x27 doesn't work)
#define LCD_COLS 16        // Number of columns
#define LCD_ROWS 2         // Number of rows

// ============================================================================
// Parking Configuration
// ============================================================================
#define TOTAL_PARKING_SLOTS 4   // Total number of parking slots
#define GATE_OPEN_TIME_MS 2000  // Time gate stays open (milliseconds)

// ============================================================================
// Servo Positions
// ============================================================================
#define SERVO_CLOSED_ANGLE 90   // Angle for closed gate
#define SERVO_OPEN_ANGLE 0      // Angle for open gate

// ============================================================================
// Time Configuration
// ============================================================================
#define TIME_API_URL "https://timeapi.io/api/Time/current/zone"
#define TIME_ZONE "Africa/Cairo"
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 7200     // GMT+2 for Cairo
#define DAYLIGHT_OFFSET_SEC 0

// ============================================================================
// Task Configuration (FreeRTOS)
// ============================================================================
// Core Assignment
#define HARDWARE_CORE 0    // Core for hardware tasks (pro_cpu)
#define COMM_CORE 1        // Core for communication tasks (app_cpu)

// Stack Sizes (in bytes)
#define SENSOR_TASK_STACK 4096
#define GATE_TASK_STACK 4096
#define LED_TASK_STACK 2048
#define DHT_TASK_STACK 3072
#define LCD_TASK_STACK 4096
#define WEB_TASK_STACK 8192
#define TELEGRAM_TASK_STACK 8192
#define WIFI_TASK_STACK 6144

// Task Priorities (higher = more priority)
#define SENSOR_TASK_PRIORITY 3
#define GATE_TASK_PRIORITY 2
#define LED_TASK_PRIORITY 1
#define DHT_TASK_PRIORITY 1
#define LCD_TASK_PRIORITY 1
#define WEB_TASK_PRIORITY 1
#define TELEGRAM_TASK_PRIORITY 1
#define WIFI_TASK_PRIORITY 1

// ============================================================================
// Timing Intervals (milliseconds)
// ============================================================================
#define SENSOR_CHECK_INTERVAL 50
#define DHT_READ_INTERVAL 2000
#define LCD_UPDATE_INTERVAL 500
#define WIFI_CHECK_INTERVAL 10000
#define TIME_UPDATE_INTERVAL 5000
#define TELEGRAM_CHECK_INTERVAL 1000
#define WEB_SERVER_INTERVAL 10

// ============================================================================
// Queue Sizes
// ============================================================================
#define ENTRY_QUEUE_SIZE 5
#define EXIT_QUEUE_SIZE 5
#define LCD_QUEUE_SIZE 10

// ============================================================================
// Debug Configuration
// ============================================================================
#define DEBUG_SERIAL true
#define SERIAL_BAUD_RATE 115200

#endif // CONFIG_H
