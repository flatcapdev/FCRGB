#include <Arduino.h>
#include <rom/rtc.h>
#include <ArduinoNvs.h>
#ifdef ESP32
  #include <esp_wifi.h>
  #include <WiFi.h>
  #include <WiFiClient.h>

  #define ESP_getChipId()   ((uint32_t)ESP.getEfuseMac())

  #define LED_ON      HIGH
  #define LED_OFF     LOW
  #define ONBOARD_LED 2
#else
  #include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
  //needed for library
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>

  #define ESP_getChipId()   (ESP.getChipId())

  #define LED_ON      LOW
  #define LED_OFF     HIGH
  #define ONBOARD_LED 2
#endif
#include <ESP_WiFiManager.h>
#include <ArduinoJson.h>

#include <FastLED.h>

#define debugSerialEnabled 1

// LEDs
#define NUM_LEDS 14
#define LED_DATA_PIN 12
CRGB leds[NUM_LEDS];

// OPTIONAL: Assign default values here.
char wifiPass[64] = ""; // when updating, but that's probably OK because they will be saved in EEPROM.
char wifiSSID[32] = ""; // Leave unset for wireless autoconfig. Note that these values will be lost

// These defaults may be overwritten with values saved by the web interface
// TODO: Add MQTT stuff later
char fcrgbNode[16] = "fcrgb01";

// Other variables
const unsigned long connectTimeout = 300;           // Timeout for WiFi and MQTT connection attempts in seconds
byte espMac[6];                                     // Byte array to store our MAC address
const float fcrgbVersion = 0.01;                    // current version
const unsigned long reConnectTimeout = 15;          // Timeout for WiFi reconnection attempts in seconds
bool shouldSaveConfig = false;                      // Flag to save json config to SPIFFS
const char wifiConfigAP[16] = "FCRGB";              // First-time config SSID
const char wifiConfigPass[16] = "fcrgbcontroller";  // First-time config WPA2 password

// Additional CSS style to match Hass theme
const char FCRGB_STYLE[] = "<style>button{background-color:#f4ac03;}body{width:60%;margin:auto;}input:invalid{border:1px solid red;}input[type=checkbox]{width:20px;}</style>";

void configRead();
void configSave();
void configSaveCallback();
void debugPrintln(String debugText);
void espWifiConfigCallback(ESP_WiFiManager *myWiFiManager);
void espWifiReconnect();
void espWifiSetup();
void handleRainbow();
void ledsSetup();