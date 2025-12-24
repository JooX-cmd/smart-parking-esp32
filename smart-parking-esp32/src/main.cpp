/**
 * @file main.cpp
 * @brief Smart Parking System with FreeRTOS on ESP32
 * @author  (Joox)
 * @date 2025
 * 
 * Features:
 * - Real-time parking slot management
 * - Web dashboard with live updates
 * - Telegram bot for remote monitoring
 * - Environmental monitoring (DHT22)
 * - Automatic barrier gate control
 * - 8 concurrent FreeRTOS tasks on dual cores
 */

// ============================================================================
// Libraries
// ============================================================================
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "DHT.h"
#include "config.h"  // Configuration file

// ============================================================================
// Hardware Pin Definitions (from config.h or fallback)
// ============================================================================
#ifndef IR_ENTRY_PIN
    #define IR_ENTRY_PIN 18
#endif
#ifndef IR_EXIT_PIN
    #define IR_EXIT_PIN 19
#endif
#ifndef SERVO_PIN
    #define SERVO_PIN 25
#endif
#ifndef GREEN_LED_PIN
    #define GREEN_LED_PIN 26
#endif
#ifndef RED_LED_PIN
    #define RED_LED_PIN 27
#endif
#ifndef DHT_PIN
    #define DHT_PIN 4
#endif
#ifndef DHT_TYPE
    #define DHT_TYPE DHT22
#endif

// ============================================================================
// Configuration Constants
// ============================================================================
#ifndef TOTAL_PARKING_SLOTS
    #define TOTAL_PARKING_SLOTS 4
#endif
#ifndef TIME_API_URL
    #define TIME_API_URL "https://timeapi.io/api/Time/current/zone"
#endif
#ifndef TIME_ZONE
    #define TIME_ZONE "Africa/Cairo"
#endif
#ifndef NTP_SERVER
    #define NTP_SERVER "pool.ntp.org"
#endif
#ifndef GMT_OFFSET_SEC
    #define GMT_OFFSET_SEC 7200
#endif
#ifndef DAYLIGHT_OFFSET_SEC
    #define DAYLIGHT_OFFSET_SEC 0
#endif

// WiFi Credentials (use config.h or define here)
#ifndef WIFI_SSID
    const char* ssid = "your_wifi_ssid";
#else
    const char* ssid = WIFI_SSID;
#endif

#ifndef WIFI_PASSWORD
    const char* password = "your_wifi_password";
#else
    const char* password = WIFI_PASSWORD;
#endif

// Telegram Bot Token
#ifndef BOT_TOKEN
    #define BOT_TOKEN "your_telegram_bot_token"
#endif

// FreeRTOS Core Assignment
static const BaseType_t pro_cpu = 0;  // Hardware tasks
static const BaseType_t app_cpu = 1;  // Communication tasks

// ============================================================================
// Task Handles
// ============================================================================
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t gateTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t lcdTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t dhtTaskHandle = NULL;
TaskHandle_t telegramTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;

// ============================================================================
// FreeRTOS Synchronization Primitives
// ============================================================================
// Queues for inter-task communication
QueueHandle_t entryQueue;
QueueHandle_t exitQueue;
QueueHandle_t lcdQueue;

// Mutexes for thread-safe access
SemaphoreHandle_t slotsMutex;
SemaphoreHandle_t gateStatusMutex;
SemaphoreHandle_t lcdMutex;
SemaphoreHandle_t dhtMutex;
SemaphoreHandle_t timeMutex;

// ============================================================================
// Global Variables (Protected by Mutexes)
// ============================================================================
const int TOTAL_SLOTS = TOTAL_PARKING_SLOTS;
int availableSlots = TOTAL_PARKING_SLOTS;
String gateStatus = "Closed";
float temperature = 0.0;
float humidity = 0.0;
String currentTime = "00:00:00";
String currentDate = "2024/01/01";
bool wifiConnected = false;
bool internetConnected = false;

// ============================================================================
// Hardware Objects
// ============================================================================
Servo barrierServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ============================================================================
// Custom Types
// ============================================================================
typedef enum {
    EVENT_CAR_ENTRY,
    EVENT_CAR_EXIT,
    EVENT_GATE_OPEN,
    EVENT_GATE_CLOSE,
    EVENT_PARKING_FULL
} EventType;

typedef struct {
    EventType type;
    int value;
} SystemEvent;

typedef struct {
    char line1[17];
    char line2[17];
} LCDMessage;

// ============================================================================
// Function Declarations
// ============================================================================
// Time functions
bool checkInternetConnection();
void initTime(const char *ntpServer, long gmtOffsetSec, int daylightOffsetSec);
String getTimeFromAPI();
String getDateFromAPI();
String getCurrentTime();
String getCurrentDate();

// Task functions
void wifiTask(void *parameter);
void sensorTask(void *parameter);
void gateTask(void *parameter);
void ledTask(void *parameter);
void dhtTask(void *parameter);
void lcdTask(void *parameter);
void webServerTask(void *parameter);
void telegramTask(void *parameter);

// Web server handlers
void handleData();
String getHTML();

// ============================================================================
// TIME FUNCTIONS
// ============================================================================

/**
 * @brief Check if internet is accessible
 * @return true if connected to internet
 */
bool checkInternetConnection() {
    if (WiFi.status() != WL_CONNECTED) return false;
    
    HTTPClient http;
    http.begin("http://clients3.google.com/generate_204");
    http.setTimeout(3000);
    int httpCode = http.GET();
    http.end();
    
    return (httpCode == 204);
}

/**
 * @brief Initialize NTP time synchronization
 */
void initTime(const char *ntpServer, long gmtOffsetSec, int daylightOffsetSec) {
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
    Serial.println("[Time] NTP configured");
}

/**
 * @brief Get time from external API (fallback if NTP fails)
 */
String getTimeFromAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        return "00:00:00";
    }
    
    HTTPClient http;
    String url = String(TIME_API_URL) + "?timeZone=" + TIME_ZONE;
    
    http.begin(url);
    http.setTimeout(5000);
    http.addHeader("Accept", "application/json");
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        
        if (!deserializeJson(doc, payload)) {
            int hour = doc["hour"];
            int minute = doc["minute"];
            int seconds = doc["seconds"];
            
            char formattedTime[20];
            sprintf(formattedTime, "%02d:%02d:%02d", hour, minute, seconds);
            
            http.end();
            return String(formattedTime);
        }
    }
    
    http.end();
    return "00:00:00";
}

/**
 * @brief Get date from external API (fallback if NTP fails)
 */
String getDateFromAPI() {
    if (WiFi.status() != WL_CONNECTED) {
        return "2024/01/01";
    }
    
    HTTPClient http;
    String url = String(TIME_API_URL) + "?timeZone=" + TIME_ZONE;
    
    http.begin(url);
    http.setTimeout(5000);
    http.addHeader("Accept", "application/json");
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        
        if (!deserializeJson(doc, payload)) {
            int year = doc["year"];
            int month = doc["month"];
            int day = doc["day"];
            
            char formattedDate[20];
            sprintf(formattedDate, "%04d/%02d/%02d", year, month, day);
            
            http.end();
            return String(formattedDate);
        }
    }
    
    http.end();
    return "2024/01/01";
}

/**
 * @brief Get current time (NTP first, then API fallback)
 */
String getCurrentTime() {
    struct tm timeinfo;
    
    if (getLocalTime(&timeinfo, 1000)) {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
        return String(buffer);
    }
    
    return getTimeFromAPI();
}

/**
 * @brief Get current date (NTP first, then API fallback)
 */
String getCurrentDate() {
    struct tm timeinfo;
    
    if (getLocalTime(&timeinfo, 1000)) {
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y/%m/%d", &timeinfo);
        return String(buffer);
    }
    
    return getDateFromAPI();
}

// ============================================================================
// FREERTOS TASKS
// ============================================================================

/**
 * @brief WiFi monitoring and time update task
 * Runs on Core 1 (Communication)
 */
void wifiTask(void *parameter) {
    unsigned long lastTimeUpdate = 0;
    unsigned long lastWiFiCheck = 0;
    const unsigned long TIME_UPDATE_INTERVAL = 5000;
    const unsigned long WIFI_CHECK_INTERVAL = 10000;
    
    while(1) {
        unsigned long now = millis();
        
        // Check WiFi connection periodically
        if(now - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
            bool prevWiFi = wifiConnected;
            wifiConnected = (WiFi.status() == WL_CONNECTED);
            
            if(wifiConnected != prevWiFi) {
                if(wifiConnected) {
                    Serial.println("[WiFi] Connected!");
                } else {
                    Serial.println("[WiFi] Disconnected! Reconnecting...");
                    WiFi.reconnect();
                }
            }
            
            if(wifiConnected) {
                internetConnected = checkInternetConnection();
            } else {
                internetConnected = false;
            }
            
            lastWiFiCheck = now;
        }
        
        // Update time periodically
        if(now - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
            if(wifiConnected) {
                String newTime = getCurrentTime();
                String newDate = getCurrentDate();
                
                if(xSemaphoreTake(timeMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    currentTime = newTime;
                    currentDate = newDate;
                    xSemaphoreGive(timeMutex);
                }
            }
            
            lastTimeUpdate = now;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief IR sensor monitoring task
 * Runs on Core 0 (Hardware) - Highest Priority
 */
void sensorTask(void *parameter) {
    SystemEvent event;
    pinMode(IR_ENTRY_PIN, INPUT);
    pinMode(IR_EXIT_PIN, INPUT);
    bool entryDetected = false;
    bool exitDetected = false;
    
    Serial.println("[Sensor] Started on Core 0");
    
    while(1) {
        // Check entry sensor (LOW = car detected)
        if(digitalRead(IR_ENTRY_PIN) == LOW && !entryDetected) {
            entryDetected = true;
            event.type = EVENT_CAR_ENTRY;
            event.value = 1;
            xQueueSend(entryQueue, &event, pdMS_TO_TICKS(100));
            
            Serial.println("\n[Sensor] CAR DETECTED AT ENTRY!");
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if(digitalRead(IR_ENTRY_PIN) == HIGH) entryDetected = false;
        
        // Check exit sensor (LOW = car detected)
        if(digitalRead(IR_EXIT_PIN) == LOW && !exitDetected) {
            exitDetected = true;
            event.type = EVENT_CAR_EXIT;
            event.value = 1;
            xQueueSend(exitQueue, &event, pdMS_TO_TICKS(100));
            
            Serial.println("\n[Sensor] CAR DETECTED AT EXIT!");
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        if(digitalRead(IR_EXIT_PIN) == HIGH) exitDetected = false;
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Gate control task - handles entry/exit events
 * Runs on Core 0 (Hardware)
 */
void gateTask(void *parameter) {
    SystemEvent event;
    LCDMessage lcdMsg;
    
    Serial.println("[Gate] Started on Core 0");
    
    while(1) {
        // Process entry events
        if(xQueueReceive(entryQueue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
                if(availableSlots > 0) {
                    Serial.printf("[Gate] ENTRY - Slots: %d/%d\n", availableSlots, TOTAL_SLOTS);
                    
                    xSemaphoreGive(slotsMutex);
                    
                    // Update gate status
                    if(xSemaphoreTake(gateStatusMutex, portMAX_DELAY) == pdTRUE) {
                        gateStatus = "Open";
                        xSemaphoreGive(gateStatusMutex);
                    }
                    
                    // Update LCD
                    strcpy(lcdMsg.line1, "Gate: OPEN");
                    strcpy(lcdMsg.line2, "Entering...");
                    xQueueSend(lcdQueue, &lcdMsg, pdMS_TO_TICKS(100));
                    
                    // Open barrier
                    Serial.println("  Opening barrier (0 degrees)...");
                    barrierServo.write(0);
                    
                    // Wait for car to pass
                    Serial.println("  Waiting 2 seconds for car passage...");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    
                    // Close barrier
                    Serial.println("  Closing barrier (90 degrees)...");
                    barrierServo.write(90);
                    
                    // Update gate status
                    if(xSemaphoreTake(gateStatusMutex, portMAX_DELAY) == pdTRUE) {
                        gateStatus = "Closed";
                        xSemaphoreGive(gateStatusMutex);
                    }
                    
                    // Decrement available slots
                    if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
                        availableSlots--;
                        Serial.printf("  Entry complete! New slots: %d/%d\n", availableSlots, TOTAL_SLOTS);
                        
                        if(availableSlots == 0) {
                            Serial.println("  PARKING NOW FULL!");
                        }
                        
                        xSemaphoreGive(slotsMutex);
                    }
                    Serial.println("");
                } else {
                    Serial.println("[Gate] PARKING FULL - Entry DENIED!\n");
                    xSemaphoreGive(slotsMutex);
                }
            }
        }
        
        // Process exit events
        if(xQueueReceive(exitQueue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {
            Serial.println("[Gate] EXIT PROCESSING");
            
            // Update gate status
            if(xSemaphoreTake(gateStatusMutex, portMAX_DELAY) == pdTRUE) {
                gateStatus = "Open";
                xSemaphoreGive(gateStatusMutex);
            }
            
            // Update LCD
            strcpy(lcdMsg.line1, "Gate: OPEN");
            strcpy(lcdMsg.line2, "Exiting...");
            xQueueSend(lcdQueue, &lcdMsg, pdMS_TO_TICKS(100));
            
            // Open barrier
            Serial.println("  Opening barrier (0 degrees)...");
            barrierServo.write(0);
            
            // Wait for car to pass
            Serial.println("  Waiting 2 seconds...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            // Close barrier
            Serial.println("  Closing barrier (90 degrees)...");
            barrierServo.write(90);
            
            // Update gate status
            if(xSemaphoreTake(gateStatusMutex, portMAX_DELAY) == pdTRUE) {
                gateStatus = "Closed";
                xSemaphoreGive(gateStatusMutex);
            }
            
            // Increment available slots
            if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
                if(availableSlots < TOTAL_SLOTS) {
                    availableSlots++;
                    Serial.printf("  Exit complete! New slots: %d/%d\n", availableSlots, TOTAL_SLOTS);
                }
                xSemaphoreGive(slotsMutex);
            }
            Serial.println("");
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief LED indicator task
 * Runs on Core 0 (Hardware)
 */
void ledTask(void *parameter) {
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    
    Serial.println("[LED] Started on Core 0");
    
    while(1) {
        if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
            // Green = slots available, Red = parking full
            digitalWrite(GREEN_LED_PIN, availableSlots > 0 ? HIGH : LOW);
            digitalWrite(RED_LED_PIN, availableSlots > 0 ? LOW : HIGH);
            xSemaphoreGive(slotsMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief DHT22 temperature/humidity sensor task
 * Runs on Core 0 (Hardware)
 */
void dhtTask(void *parameter) {
    Serial.println("[DHT] Started on Core 0");
    
    float lastTemp = -999;
    float lastHum = -999;
    bool firstReading = true;
    
    while(1) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        if (!isnan(h) && !isnan(t)) {
            if(xSemaphoreTake(dhtMutex, portMAX_DELAY) == pdTRUE) {
                temperature = t;
                humidity = h;
                xSemaphoreGive(dhtMutex);
                
                // Only log significant changes
                if(firstReading || abs(t - lastTemp) > 0.5 || abs(h - lastHum) > 2.0) {
                    Serial.printf("[DHT] Temp: %.1fC | Humidity: %.1f%%\n", t, h);
                    lastTemp = t;
                    lastHum = h;
                    firstReading = false;
                }
            }
        } else {
            Serial.println("[DHT] Invalid reading - check wiring");
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief LCD display task
 * Runs on Core 1 (Communication)
 */
void lcdTask(void *parameter) {
    LCDMessage msg;
    TickType_t lastUpdate = 0;
    const TickType_t updateInterval = pdMS_TO_TICKS(500);
    
    Serial.println("[LCD] Started on Core 1");
    
    while(1) {
        // Check for priority messages in queue
        if(xQueueReceive(lcdQueue, &msg, pdMS_TO_TICKS(10)) == pdTRUE) {
            if(xSemaphoreTake(lcdMutex, portMAX_DELAY) == pdTRUE) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(msg.line1);
                lcd.setCursor(0, 1);
                lcd.print(msg.line2);
                xSemaphoreGive(lcdMutex);
            }
            lastUpdate = xTaskGetTickCount();
        }
        // Default display update
        else if((xTaskGetTickCount() - lastUpdate) >= updateInterval) {
            if(xSemaphoreTake(lcdMutex, portMAX_DELAY) == pdTRUE) {
                lcd.clear();
                
                // Line 1: Time and slots
                lcd.setCursor(0, 0);
                String timeStr = "";
                if(xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    timeStr = currentTime;
                    xSemaphoreGive(timeMutex);
                }
                lcd.print(timeStr);
                lcd.print(" ");
                
                if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
                    lcd.print(availableSlots);
                    xSemaphoreGive(slotsMutex);
                }
                lcd.print("/");
                lcd.print(TOTAL_SLOTS);
                
                // Line 2: Gate status
                lcd.setCursor(0, 1);
                lcd.print("Gate:");
                if(xSemaphoreTake(gateStatusMutex, portMAX_DELAY) == pdTRUE) {
                    lcd.print(gateStatus);
                    xSemaphoreGive(gateStatusMutex);
                }
                
                xSemaphoreGive(lcdMutex);
            }
            lastUpdate = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Web server task
 * Runs on Core 1 (Communication)
 */
void webServerTask(void *parameter) {
    Serial.println("[Web] Started on Core 1");
    
    while(1) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Telegram bot task
 * Runs on Core 1 (Communication)
 */
void telegramTask(void *parameter) {
    Serial.println("[Telegram] Started on Core 1");
    
    while(1) {
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        
        for(int i = 0; i < numNewMessages; i++) {
            String chat_id = bot.messages[i].chat_id;
            String text = bot.messages[i].text;
            
            Serial.printf("[Telegram] Received command: %s\n", text.c_str());
            
            if(text == "/start") {
                String msg = "*🚗 FreeRTOS Parking System*\n\n";
                msg += "Available Commands:\n";
                msg += "/status - Parking status\n";
                msg += "/time - Date & Time\n";
                msg += "/temp - Temperature\n";
                msg += "/all - Complete info";
                bot.sendMessage(chat_id, msg, "Markdown");
            }
            else if(text == "/status") {
                String msg = "*🅿️ Parking Status*\n\n";
                if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
                    msg += "Available: " + String(availableSlots) + "/" + String(TOTAL_SLOTS);
                    if(availableSlots == 0) msg += " ❌ FULL";
                    else msg += " ✅";
                    xSemaphoreGive(slotsMutex);
                }
                bot.sendMessage(chat_id, msg, "Markdown");
            }
            else if(text == "/time") {
                String msg = "*🕒 Date & Time*\n\n";
                if(xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    msg += "📅 " + currentDate + "\n";
                    msg += "⏰ " + currentTime;
                    xSemaphoreGive(timeMutex);
                }
                bot.sendMessage(chat_id, msg, "Markdown");
            }
            else if(text == "/temp") {
                String msg = "*🌡️ Environment*\n\n";
                if(xSemaphoreTake(dhtMutex, portMAX_DELAY) == pdTRUE) {
                    msg += "Temperature: " + String(temperature, 1) + "°C\n";
                    msg += "Humidity: " + String(humidity, 1) + "%";
                    xSemaphoreGive(dhtMutex);
                }
                bot.sendMessage(chat_id, msg, "Markdown");
            }
            else if(text == "/all") {
                String msg = "*📊 Complete Status*\n\n";
                if(xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    msg += "📅 " + currentDate + " " + currentTime + "\n\n";
                    xSemaphoreGive(timeMutex);
                }
                if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
                    msg += "🅿️ Parking: " + String(availableSlots) + "/" + String(TOTAL_SLOTS) + "\n";
                    xSemaphoreGive(slotsMutex);
                }
                if(xSemaphoreTake(dhtMutex, portMAX_DELAY) == pdTRUE) {
                    msg += "🌡️ Temp: " + String(temperature, 1) + "°C\n";
                    msg += "💧 Humidity: " + String(humidity, 1) + "%";
                    xSemaphoreGive(dhtMutex);
                }
                bot.sendMessage(chat_id, msg, "Markdown");
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================================
// WEB SERVER HANDLERS
// ============================================================================

/**
 * @brief Handle /data endpoint - returns JSON with all sensor data
 */
void handleData() {
    String json = "{";
    
    if(xSemaphoreTake(slotsMutex, portMAX_DELAY) == pdTRUE) {
        json += "\"available\":" + String(availableSlots) + ",";
        json += "\"occupied\":" + String(TOTAL_SLOTS - availableSlots) + ",";
        xSemaphoreGive(slotsMutex);
    }
    
    if(xSemaphoreTake(gateStatusMutex, portMAX_DELAY) == pdTRUE) {
        json += "\"gate\":\"" + gateStatus + "\",";
        xSemaphoreGive(gateStatusMutex);
    }
    
    if(xSemaphoreTake(dhtMutex, portMAX_DELAY) == pdTRUE) {
        json += "\"temperature\":" + String(temperature, 1) + ",";
        json += "\"humidity\":" + String(humidity, 1) + ",";
        xSemaphoreGive(dhtMutex);
    }
    
    if(xSemaphoreTake(timeMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        json += "\"time\":\"" + currentTime + "\",";
        json += "\"date\":\"" + currentDate + "\",";
        xSemaphoreGive(timeMutex);
    }
    
    json += "\"wifi\":" + String(wifiConnected ? "true" : "false") + ",";
    json += "\"internet\":" + String(internetConnected ? "true" : "false") + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    
    json += "}";
    server.send(200, "application/json", json);
}

/**
 * @brief Generate HTML for web dashboard
 */
String getHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1.0'>
    <title>FreeRTOS Parking</title>
    <style>
        *{margin:0;padding:0;box-sizing:border-box}
        body{font-family:'Segoe UI',Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;min-height:100vh;padding:20px}
        .container{max-width:1200px;margin:0 auto}
        h1{text-align:center;font-size:2.5em;margin-bottom:10px;text-shadow:2px 2px 4px rgba(0,0,0,0.3)}
        .subtitle{text-align:center;font-size:1.1em;opacity:0.9;margin-bottom:30px}
        .status-bar{background:rgba(255,255,255,0.1);border-radius:15px;padding:15px;margin-bottom:20px;display:flex;justify-content:space-around;flex-wrap:wrap;backdrop-filter:blur(10px)}
        .status-item{text-align:center;padding:10px}
        .status-item .label{font-size:0.9em;opacity:0.8;margin-bottom:5px}
        .status-item .value{font-size:1.3em;font-weight:bold}
        .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin:20px 0}
        .card{background:rgba(255,255,255,0.15);border-radius:20px;padding:25px;text-align:center;backdrop-filter:blur(10px);border:2px solid rgba(255,255,255,0.2);transition:transform 0.3s,box-shadow 0.3s}
        .card:hover{transform:translateY(-5px);box-shadow:0 10px 30px rgba(0,0,0,0.3)}
        .icon{font-size:3em;margin-bottom:15px}
        .label{font-size:1em;opacity:0.8;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px}
        .stat{font-size:3em;font-weight:bold;margin:10px 0;text-shadow:2px 2px 4px rgba(0,0,0,0.2)}
        .unit{font-size:0.5em;opacity:0.9}
        .badge{display:inline-block;background:rgba(255,255,255,0.2);padding:8px 15px;border-radius:20px;font-size:0.9em;margin-top:10px}
        .green{color:#00ff88}
        .red{color:#ff4444}
        .blue{color:#4488ff}
        .orange{color:#ff8844}
        .pulse{animation:pulse 2s infinite}
        @keyframes pulse{0%,100%{opacity:1}50%{opacity:0.6}}
        .footer{text-align:center;margin-top:30px;padding:20px;opacity:0.7;font-size:0.9em}
        @media(max-width:768px){h1{font-size:2em}.stat{font-size:2.5em}.grid{grid-template-columns:1fr}}
    </style>
</head>
<body>
    <div class='container'>
        <h1>🚗 FreeRTOS Parking System</h1>
        <div class='subtitle'>Real-Time Multitasking Dashboard</div>
        
        <div class='status-bar'>
            <div class='status-item'><div class='label'>📅 Date</div><div class='value' id='date'>--</div></div>
            <div class='status-item'><div class='label'>🕒 Time</div><div class='value' id='time'>--</div></div>
            <div class='status-item'><div class='label'>📡 WiFi</div><div class='value' id='wifi'>--</div></div>
            <div class='status-item'><div class='label'>🌐 Internet</div><div class='value' id='internet'>--</div></div>
            <div class='status-item'><div class='label'>⏱️ Uptime</div><div class='value' id='uptime'>--</div></div>
        </div>
        
        <div class='grid'>
            <div class='card'><div class='icon'>🅿️</div><div class='label'>Available Slots</div><div id='available' class='stat green'>0</div><div class='badge'>Spaces Free</div></div>
            <div class='card'><div class='icon'>🚙</div><div class='label'>Occupied</div><div id='occupied' class='stat red'>0</div><div class='badge'>Cars Inside</div></div>
            <div class='card'><div class='icon'>🚧</div><div class='label'>Gate Status</div><div id='gate' class='stat red'>Closed</div><div class='badge' id='gateBadge'>Barrier Down</div></div>
            <div class='card'><div class='icon'>🌡️</div><div class='label'>Temperature</div><div id='temp' class='stat orange'>--<span class='unit'>°C</span></div><div class='badge'>Live Data</div></div>
            <div class='card'><div class='icon'>💧</div><div class='label'>Humidity</div><div id='humid' class='stat blue'>--<span class='unit'>%</span></div><div class='badge'>Live Data</div></div>
            <div class='card'><div class='icon'>⚡</div><div class='label'>System Status</div><div class='stat green'>ONLINE</div><div class='badge pulse'>8 Tasks Running</div></div>
        </div>
        
        <div class='footer'>Powered by ESP32 FreeRTOS | 8 Concurrent Tasks | Dual Core Processing</div>
    </div>
    
    <script>
        function formatUptime(sec) {
            const h = Math.floor(sec / 3600);
            const m = Math.floor((sec % 3600) / 60);
            const s = sec % 60;
            return h + 'h ' + m + 'm ' + s + 's';
        }
        
        async function update() {
            try {
                const r = await fetch('/data');
                const d = await r.json();
                
                document.getElementById('available').innerText = d.available;
                document.getElementById('occupied').innerText = d.occupied;
                
                const g = document.getElementById('gate');
                const gb = document.getElementById('gateBadge');
                g.innerText = d.gate;
                if (d.gate == 'Open') {
                    g.className = 'stat green';
                    gb.innerText = 'Barrier Up';
                } else {
                    g.className = 'stat red';
                    gb.innerText = 'Barrier Down';
                }
                
                document.getElementById('temp').innerHTML = d.temperature + "<span class='unit'>°C</span>";
                document.getElementById('humid').innerHTML = d.humidity + "<span class='unit'>%</span>";
                document.getElementById('date').innerText = d.date;
                document.getElementById('time').innerText = d.time;
                document.getElementById('wifi').innerText = d.wifi ? '✅ Connected' : '❌ Offline';
                document.getElementById('internet').innerText = d.internet ? '✅ Online' : '❌ Offline';
                document.getElementById('uptime').innerText = formatUptime(d.uptime);
            } catch (e) {
                console.error(e);
            }
        }
        
        setInterval(update, 1000);
        update();
    </script>
</body>
</html>
)rawliteral";
    
    return html;
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("   SMART PARKING SYSTEM - FreeRTOS");
    Serial.println("========================================\n");
    
    // Initialize I2C and LCD
    Serial.println("[Hardware] Initializing...");
    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("FreeRTOS Parking");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    
    // Initialize servo
    barrierServo.attach(SERVO_PIN);
    barrierServo.write(90);  // Start closed
    Serial.println("[Servo] Attached to GPIO 25 (90 deg - Closed)");
    
    // Initialize DHT sensor
    dht.begin();
    Serial.println("[DHT22] Sensor initialized on GPIO 4");
    
    // Configure Telegram secure client
    secured_client.setInsecure();
    Serial.println("[Telegram] Secure client configured");
    
    // Connect to WiFi
    Serial.println("");
    Serial.print("[WiFi] Connecting to: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if(WiFi.status() == WL_CONNECTED) {
        Serial.print("[WiFi] Connected! IP: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        
        // Initialize NTP
        initTime(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);
        delay(2000);
    } else {
        Serial.println("[WiFi] Connection failed!");
        wifiConnected = false;
    }
    
    // Setup web server routes
    server.on("/", []() { 
        server.send(200, "text/html", getHTML()); 
    });
    server.on("/data", handleData);
    server.begin();
    Serial.print("[Web] Server started at http://");
    Serial.println(WiFi.localIP());
    
    // Create mutexes
    Serial.println("\n[RTOS] Creating synchronization primitives...");
    slotsMutex = xSemaphoreCreateMutex();
    gateStatusMutex = xSemaphoreCreateMutex();
    lcdMutex = xSemaphoreCreateMutex();
    dhtMutex = xSemaphoreCreateMutex();
    timeMutex = xSemaphoreCreateMutex();
    
    // Create queues
    entryQueue = xQueueCreate(5, sizeof(SystemEvent));
    exitQueue = xQueueCreate(5, sizeof(SystemEvent));
    lcdQueue = xQueueCreate(10, sizeof(LCDMessage));
    
    // Create tasks
    Serial.println("[RTOS] Creating tasks...\n");
    
    // Core 0 tasks (Hardware)
    xTaskCreatePinnedToCore(sensorTask, "Sensor", 4096, NULL, 3, &sensorTaskHandle, pro_cpu);
    xTaskCreatePinnedToCore(dhtTask, "DHT", 3072, NULL, 1, &dhtTaskHandle, pro_cpu);
    xTaskCreatePinnedToCore(gateTask, "Gate", 4096, NULL, 2, &gateTaskHandle, pro_cpu);
    xTaskCreatePinnedToCore(ledTask, "LED", 2048, NULL, 1, &ledTaskHandle, pro_cpu);
    
    // Core 1 tasks (Communication)
    xTaskCreatePinnedToCore(lcdTask, "LCD", 4096, NULL, 1, &lcdTaskHandle, app_cpu);
    xTaskCreatePinnedToCore(webServerTask, "Web", 8192, NULL, 1, &webServerTaskHandle, app_cpu);
    xTaskCreatePinnedToCore(telegramTask, "Telegram", 8192, NULL, 1, &telegramTaskHandle, app_cpu);
    xTaskCreatePinnedToCore(wifiTask, "WiFi", 6144, NULL, 1, &wifiTaskHandle, app_cpu);
    
    Serial.println("========================================");
    Serial.println("   All 8 tasks created successfully!");
    Serial.println("   Waiting for sensor events...");
    Serial.println("========================================\n");
    
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System Ready!");
    lcd.setCursor(0, 1);
    lcd.print("4/4 Available");
    
    // Delete setup task (FreeRTOS takes over)
    vTaskDelete(NULL);
}

// ============================================================================
// LOOP (Empty - FreeRTOS tasks handle everything)
// ============================================================================

void loop() {
    // Empty - all work is done by FreeRTOS tasks
}
