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
#include <MQTT.h>

#define debugSerialEnabled 1

// LEDs
#define NUM_LEDS 14
#define LED_DATA_PIN 12
CRGB leds[NUM_LEDS];

// OPTIONAL: Assign default values here.
char wifiPass[64] = ""; // when updating, but that's probably OK because they will be saved in EEPROM.
char wifiSSID[32] = ""; // Leave unset for wireless autoconfig. Note that these values will be lost

// These defaults may be overwritten with values saved by the web interface
char configPassword[32] = "";
char configUser[32] = "admin";
char fcrgbNode[16] = "fcrgb01";
char mqttServer[64] = "";
char mqttPort[6] = "1883";
char mqttUser[32] = "";
char mqttPassword[32] = "";

// Other variables
const unsigned long connectTimeout = 300;           // Timeout for WiFi and MQTT connection attempts in seconds
byte espMac[6];                                     // Byte array to store our MAC address
const float fcrgbVersion = 0.01;                    // current version
String mqttClientId;                                // Auto-generated MQTT ClientID
String mqttCommandTopic;                            // MQTT topic for incoming panel commands
String mqttLightCommandTopic;                       // MQTT topic for incoming panel backlight on/off commands
String mqttLightBrightCommandTopic;                 // MQTT topic for incoming panel backlight dimmer commands
String mqttLightBrightStateTopic;                   // MQTT topic for outgoing panel backlight dimmer state
String mqttLightStateTopic;                         // MQTT topic for outgoing panel backlight on/off state
const uint16_t mqttMaxPacketSize = 4096;            // Size of buffer for incoming MQTT message
String mqttSensorTopic;                             // MQTT topic for publishing device information in JSON format
String mqttStateJSONTopic;                          // MQTT topic for outgoing panel interactions in JSON format
String mqttStateTopic;                              // MQTT topic for outgoing panel interactions
String mqttStatusTopic;                             // MQTT topic for publishing device connectivity state
const unsigned long reConnectTimeout = 15;          // Timeout for WiFi reconnection attempts in seconds
bool shouldSaveConfig = false;                      // Flag to save json config to SPIFFS
const long statusUpdateInterval = 300000;           // Time in msec between publishing MQTT status updates (5 minutes)
long statusUpdateTimer = 0;                         // Timer for update check
const char wifiConfigAP[16] = "FCRGB";              // First-time config SSID
const char wifiConfigPass[16] = "fcrgbcontroller";  // First-time config WPA2 password

MQTTClient mqttClient(mqttMaxPacketSize);
WebServer webServer(80);
WiFiClient wifiMQTTClient;

// Additional CSS style to match Hass theme
const char FCRGB_STYLE[] = "<style>button{background-color:#f4ac03;}body{width:60%;margin:auto;}input:invalid{border:1px solid red;}input[type=checkbox]{width:20px;}</style>";

void configClearSaved();
void configRead();
void configSave();
void configSaveCallback();
void debugPrintln(String debugText);
void espReset();
void espWifiConfigCallback(ESP_WiFiManager *myWiFiManager);
void espWifiReconnect();
void espWifiSetup();
void handleRainbow();
void ledsSetup();
void mqttCallback(String &strTopic, String &strPayload);
void mqttConnect();
void mqttSetup();
void mqttStatusUpdate();
void webHandleConfigReset();
void webHandleConfigSave();
void webHandleNotFound();
void webHandleReboot();
void webHandleRoot();
void webServerSetup();
