#include <main.h>

void setup()
{
#ifdef debugSerialEnabled
  Serial.begin(115200);
#endif

  debugPrintln(String(F("SYSTEM: Starting FCRGB v")) + String(fcrgbVersion));
  debugPrintln(String(F("SYSTEM: Last reset reason: ")) + String((int)rtc_get_reset_reason(0)));

  pinMode(ONBOARD_LED,OUTPUT);

  configRead(); // Check filesystem for a saved config.json

  espWifiSetup(); // Start up networking

  // TODO: OTA Updates go here

  // TODO: MQTT setup here

  ledsSetup();

  FastLED.setBrightness(32);

  debugPrintln(F("SYSTEM: System init complete."));
}

void loop()
{
  while ((WiFi.status() != WL_CONNECTED) || (WiFi.localIP().toString() == "0.0.0.0"))
  { // Check WiFi is connected and that we have a valid IP, retry until we do.
    if (WiFi.status() == WL_CONNECTED)
    { // If we're currently connected, disconnect so we can try again
      WiFi.disconnect();
    }
    espWifiReconnect();
  }

  handleRainbow();
}

// Read saved config from NVS
void configRead()
{
  debugPrintln(F("NVS: reading from NVS"));
  if (NVS.begin())
  {
    // TODO: Add MQTT stuff later
    NVS.getString("fcrgbNode").toCharArray(fcrgbNode, sizeof(fcrgbNode));
  }
  else
  {
    debugPrintln(F("NVS: [ERROR] Failed to start NVS"));
  }
}

void configSave()
{ // Save the custom parameters to NVS
  debugPrintln(F("NVS: Saving config"));

  NVS.setString("fcrgbNode", fcrgbNode);
  // TODO: MQTT stuff here

  shouldSaveConfig = false;
}

// Callback notifying us of the need to save config
void configSaveCallback()
{
  debugPrintln(F("NVS: Configuration changed, flagging for save"));
  shouldSaveConfig = true;
}

// Debug output line of text to serial with timing
void debugPrintln(String debugText)
{
#ifdef debugSerialEnabled
  String debugTimeText = "[+" + String(float(millis()) / 1000, 3) + "s] " + debugText;
  Serial.println(debugTimeText);
#endif
}

void espReset()
{
  debugPrintln(F("RESET: FCRGB reset"));

  // TODO: Update MQTT

  ESP.restart();
  delay(5000);
}

// Notify the user that we're entering config mode
void espWifiConfigCallback(ESP_WiFiManager *myWiFiManager)
{
  debugPrintln(F("WIFI: Failed to connect to assigned AP, entering config mode"));
  String msg = String("WIFI: Connect to " + String(wifiConfigAP) + " with passsword: " + String(wifiConfigPass));
  debugPrintln(msg);

  digitalWrite(ONBOARD_LED, LED_ON);
}

void espWifiReconnect()
{ // Existing WiFi connection dropped, try to reconnect
  debugPrintln(F("Reconnecting to WiFi network..."));
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPass);

  unsigned long wifiReconnectTimer = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (millis() >= (wifiReconnectTimer + (reConnectTimeout * 1000)))
    { // If we've been trying to reconnect for reConnectTimeout seconds, reboot and try again
      debugPrintln(F("WIFI: Failed to reconnect and hit timeout"));
      espReset();
    }
  }
}

// Connect to WiFi
void espWifiSetup()
{
  debugPrintln(F("WIFI: Entering Setup"));

  WiFi.macAddress(espMac);     // Read our MAC address and save it to espMac
  WiFi.setAutoReconnect(true); // Tell WiFi to autoreconnect if connection has dropped
  WiFi.setSleep(false);        // Disable WiFi sleep modes to prevent occasional disconnects

  if (String(wifiSSID) == "")
  { // If the sketch has not defined a static wifiSSID use WiFiManager to collect required information from the user.

    // id/name, placeholder/prompt, default value, length, extra tags
    ESP_WMParameter custom_fcrgbNodeHeader("<br/><br/><b>FCRGB Node Name</b>");
    ESP_WMParameter custom_fcrgbNode("fcrgbNode", "FCRGB Node (required. lowercase letters, numbers, and _ only)", fcrgbNode, 15, " maxlength=15 required pattern='[a-z0-9_]*'");
    // TOOD: Add MQTT stuff here later

    ESP_WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(configSaveCallback); // set config save notify callback
    wifiManager.setCustomHeadElement(FCRGB_STYLE);         // add custom style
    wifiManager.addParameter(&custom_fcrgbNodeHeader);
    wifiManager.addParameter(&custom_fcrgbNode);
    // TOOD: Add MQTT stuff here later

    // Timeout config portal after connectTimeout seconds, useful if configured wifi network was temporarily unavailable
    wifiManager.setTimeout(connectTimeout);

    wifiManager.setAPCallback(espWifiConfigCallback);

    // Fetches SSID and pass from EEPROM and tries to connect
    // If it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration.
    if (!wifiManager.autoConnect(wifiConfigAP, wifiConfigPass))
    { // Reset and try again
      debugPrintln(F("WIFI: Failed to connect and hit timeout"));
      espReset();
    }

    // Read updated parameters
    // TODO: MQTT stuff here

    if (shouldSaveConfig)
    { // Save the custom parameters to FS
      configSave();
    }
  }
  else
  { // wifiSSID has been defined, so attempt to connect to it forever
    debugPrintln(String(F("Connecting to WiFi network: ")) + String(wifiSSID));
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPass);

    unsigned long wifiReconnectTimer = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      if (millis() >= (wifiReconnectTimer + (connectTimeout * 1000)))
      { // If we've been trying to reconnect for connectTimeout seconds, reboot and try again
        debugPrintln(F("WIFI: Failed to connect and hit timeout"));
        espReset();
      }
    }
  }
  // If you get here you have connected to WiFi
  digitalWrite(ONBOARD_LED, LED_OFF);
}

void handleRainbow() 
{
  uint8_t hueRate = 100; // Effects cycle rate. (range >0 to 255)
  fill_rainbow( leds, NUM_LEDS, millis()/hueRate); // Start hue effected by time.
  FastLED.show();
}

void ledsSetup()
{
  debugPrintln(F("ledsSetup"));
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS); // GRB ordering is assumed
}