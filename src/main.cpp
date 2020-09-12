#include <main.h>

void setup()
{
#ifdef debugSerialEnabled
  Serial.begin(115200);
#endif

  debugPrintln(String(F("SYSTEM: Starting FCRGB v")) + String(fcrgbVersion));
  debugPrintln(String(F("SYSTEM: Last reset reason: ")) + String((int)rtc_get_reset_reason(0)));

  pinMode(ONBOARD_LED, OUTPUT);

  configRead(); // Check filesystem for a saved config.json

  espWifiSetup(); // Start up networking

  webServerSetup(); // Start up web server

  espSetupOta();

  mqttSetup();

  ledsSetup();

  debugPrintln(F("SYSTEM: System init complete."));
  mqttSensorUpdate();
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

  if (!mqttClient.connected())
  { // Check MQTT connection
    debugPrintln("MQTT: not connected, connecting.");
    mqttConnect();
  }

  mqttClient.loop();        // MQTT client loop
  ArduinoOTA.handle();      // Arduino OTA loop
  webServer.handleClient(); // webServer loop

  if ((millis() - statusUpdateTimer) >= statusUpdateInterval)
  { // Run periodic status update
    statusUpdateTimer = millis();
    mqttSensorUpdate();
  }

  if (String("christmas") == effect)
  {
    handleRWBGlitter(leds, atoi(ledsToUse), true);
  }
  else if (String("cycle") == effect)
  {
    handleCycle(leds, atoi(ledsToUse));
  }
  else if (String("cylon") == effect)
  {
    handleCylon(leds, atoi(ledsToUse));
  }
  else if (String("cylon-rainbow") == effect)
  {
    handleCylon(leds, atoi(ledsToUse), true);
  }
  else if (String("disco") == effect)
  {
    handleDisco(leds, atoi(ledsToUse));
  }
  else if (String("fire") == effect)
  {
    handleFire(leds, atoi(ledsToUse));
  }
  else if (String("rainbow") == effect)
  {
    handleRainbow(leds, atoi(ledsToUse));
  }
  else if (String("rwb-glitter") == effect)
  {
    handleRWBGlitter(leds, atoi(ledsToUse));
  }
  else
  {
    ledsHandle();
  }
}

// Clear out all local storage
void configClearSaved()
{
  debugPrintln(F("RESET: Formatting NVS"));
  NVS.eraseAll();
  debugPrintln(F("RESET: Clearing WiFiManager settings..."));
  ESP_WiFiManager wifiManager;
  wifiManager.resetSettings();
  debugPrintln(F("RESET: Rebooting device"));
  espReset();
}

// Read saved config from NVS
void configRead()
{
  debugPrintln(F("NVS: reading from NVS"));
  if (NVS.begin())
  {
    NVS.getString("fcrgbNode").toCharArray(fcrgbNode, sizeof(fcrgbNode));
    NVS.getString("mqttServer").toCharArray(mqttServer, sizeof(mqttServer));
    NVS.getString("mqttPort").toCharArray(mqttPort, sizeof(mqttPort));
    NVS.getString("mqttUser").toCharArray(mqttUser, sizeof(mqttUser));
    NVS.getString("mqttPassword").toCharArray(mqttPassword, sizeof(mqttPassword));
    NVS.getString("configPassword").toCharArray(configPassword, sizeof(configPassword));
    NVS.getString("configUser").toCharArray(configUser, sizeof(configUser));
    NVS.getString("NumLEDs").toCharArray(ledsToUse, sizeof(ledsToUse));

    debugPrintln(String(F("NVS: fcrgbNode = ")) + String(fcrgbNode));
    debugPrintln(String(F("NVS: mqttServer = ")) + String(mqttServer));
    debugPrintln(String(F("NVS: mqttPort = ")) + String(mqttPort));
    debugPrintln(String(F("NVS: mqttUser = ")) + String(mqttUser));
    debugPrintln(String(F("NVS: mqttPassword = ")) + String(mqttPassword));
    debugPrintln(String(F("NVS: configUser = ")) + String(configUser));
    debugPrintln(String(F("NVS: configPassword = ")) + String(configPassword));
    debugPrintln(String(F("NWS: NumLEDs = ")) + String(ledsToUse));
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
  NVS.setString("mqttServer", mqttServer);
  NVS.setString("mqttPort", mqttPort);
  NVS.setString("mqttUser", mqttUser);
  NVS.setString("mqttPassword", mqttPassword);
  NVS.setString("configPassword", configPassword);
  NVS.setString("configUser", configUser);
  NVS.setString("NumLEDs", ledsToUse);

  NVS.commit();

  debugPrintln(String(F("NVS: fcrgbNode = ")) + String(fcrgbNode));
  debugPrintln(String(F("NVS: mqttServer = ")) + String(mqttServer));
  debugPrintln(String(F("NVS: mqttPort = ")) + String(mqttPort));
  debugPrintln(String(F("NVS: mqttUser = ")) + String(mqttUser));
  debugPrintln(String(F("NVS: mqttPassword = ")) + String(mqttPassword));
  debugPrintln(String(F("NVS: configUser = ")) + String(configUser));
  debugPrintln(String(F("NVS: configPassword = ")) + String(configPassword));
  debugPrintln(String(F("NWS: NumLEDs = ")) + String(ledsToUse));

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

  if (mqttClient.connected())
  {
    // TODO: handle this in the json file somehow...
    // mqttClient.publish(mqttTopicStatus, "OFF", true, 1);
    mqttClient.publish(mqttTopicSensor, "{\"status\": \"unavailable\"}", true, 1);
    mqttClient.disconnect();
  }

  ESP.restart();
  delay(5000);
}

// (mostly) boilerplate OTA setup from library examples
void espSetupOta()
{
  ArduinoOTA.setHostname(fcrgbNode);
  ArduinoOTA.setPassword(configPassword);

  ArduinoOTA.onStart([]() {
    debugPrintln(F("ESP OTA: update start"));
  });
  ArduinoOTA.onEnd([]() {
    debugPrintln(F("ESP OTA: update complete"));
    espReset();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debugPrintln(String(progress / (total / 100)) + "%\"");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    debugPrintln(String(F("ESP OTA: ERROR code ")) + String(error));
    if (error == OTA_AUTH_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Auth Failed"));
    else if (error == OTA_BEGIN_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Begin Failed"));
    else if (error == OTA_CONNECT_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR)
      debugPrintln(F("ESP OTA: ERROR - Receive Failed"));
    else if (error == OTA_END_ERROR)
      debugPrintln(F("ESP OTA: ERROR - End Failed"));
    delay(5000);
  });
  ArduinoOTA.begin();
  debugPrintln(F("ESP OTA: Over the Air firmware update ready"));
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

  if (String(wifiSSID) == "" || String(mqttServer) == "")
  { // If the sketch has not defined a static wifiSSID use WiFiManager to collect required information from the user.

    // id/name, placeholder/prompt, default value, length, extra tags
    ESP_WMParameter custom_fcrgbNodeHeader("<br/><br/><b>FCRGB Node Name</b>");
    ESP_WMParameter custom_fcrgbNode("fcrgbNode", "FCRGB Node (required. lowercase letters, numbers, and _ only)", fcrgbNode, 15, " maxlength=15 required pattern='[a-z0-9_]*'");
    ESP_WMParameter custom_mqttHeader("<br/><br/><b>MQTT Broker</b>");
    ESP_WMParameter custom_mqttServer("mqttServer", "MQTT Server", mqttServer, 63, " maxlength=39");
    ESP_WMParameter custom_mqttPort("mqttPort", "MQTT Port", mqttPort, 5, " maxlength=5 type='number'");
    ESP_WMParameter custom_mqttUser("mqttUser", "MQTT User", mqttUser, 31, " maxlength=31");
    ESP_WMParameter custom_mqttPassword("mqttPassword", "MQTT Password", mqttPassword, 31, " maxlength=31 type='password'");
    ESP_WMParameter custom_configPassword("configPassword", "Config Password", configPassword, 31, " maxlength=31 type='password'");
    ESP_WMParameter custom_configUser("configUser", "Config User", configUser, 15, " maxlength=31'");
    ESP_WMParameter custom_numLeds("ledsToUse", "Number of LEDS", ledsToUse, 3, " maxlength=3 type='number'");

    ESP_WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(configSaveCallback); // set config save notify callback
    wifiManager.setCustomHeadElement(FCRGB_STYLE);         // add custom style
    wifiManager.addParameter(&custom_fcrgbNodeHeader);
    wifiManager.addParameter(&custom_fcrgbNode);
    wifiManager.addParameter(&custom_mqttHeader);
    wifiManager.addParameter(&custom_mqttServer);
    wifiManager.addParameter(&custom_mqttPort);
    wifiManager.addParameter(&custom_mqttUser);
    wifiManager.addParameter(&custom_mqttPassword);
    wifiManager.addParameter(&custom_configUser);
    wifiManager.addParameter(&custom_configPassword);
    wifiManager.addParameter(&custom_numLeds);

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
    strcpy(fcrgbNode, custom_fcrgbNode.getValue());
    strcpy(mqttServer, custom_mqttServer.getValue());
    strcpy(mqttPort, custom_mqttPort.getValue());
    strcpy(mqttUser, custom_mqttUser.getValue());
    strcpy(mqttPassword, custom_mqttPassword.getValue());
    strcpy(configPassword, custom_configPassword.getValue());
    strcpy(configUser, custom_configUser.getValue());
    strcpy(ledsToUse, custom_numLeds.getValue());

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

void ledsHandle()
{
  bool lightsOnChanged = false;
  bool brightnessChanged = false;
  bool redChanged = false;
  bool greenChanged = false;
  bool blueChanged = false;

  lightsOnChanged = lastLightsOn != lightsOn;
  if (lightsOnChanged)
  {
    debugPrintln(String(F("ledsHandle: lightsOn changing (")) + String(ledsToUse) + ")");
    if (lightsOn)
    {
      debugPrintln(F("ledsHandle: lightsOn ON"));
      FastLED.setBrightness(lastBrightness);
      fill_solid(leds, atoi(ledsToUse), CRGB(lastRed, lastGreen, lastBlue));
      FastLED.show();
    }
    else
    {
      debugPrintln(F("ledsHandle: lightsOn OFF"));
      FastLED.setBrightness(0);
      FastLED.show();
    }
    lastLightsOn = lightsOn;
  }

  if (lightsOn)
  {
    brightnessChanged = lastBrightness != brightness;
    if (brightnessChanged)
    {
      debugPrintln(F("ledsHandle: brightness changing"));
      FastLED.setBrightness(brightness);
      FastLED.show();
      lastBrightness = brightness;
    }

    redChanged = lastRed != red;
    greenChanged = lastGreen != green;
    blueChanged = lastBlue != blue;

    if (redChanged || greenChanged || blueChanged)
    {
      debugPrintln(F("ledsHandle: color changing"));
      fill_solid(leds, atoi(ledsToUse), CRGB(red, green, blue));
      FastLED.show();
      char rgb[12];
      sprintf(rgb, "%d,%d,%d", red, green, blue);
      lastRed = red;
      lastGreen = green;
      lastBlue = blue;
    }
  }

  if (brightnessChanged || redChanged || greenChanged || blueChanged || lightsOnChanged)
  {
    if (brightnessChanged)
    {
      NVS.setInt("lastBrightness", lastBrightness);
      debugPrintln(String(F("LED: lastBrightness: ")) + String(lastBrightness));
    }
    if (redChanged)
    {
      NVS.setInt("lastRed", lastRed);
      debugPrintln(String(F("LED: lastRed: ")) + String(lastRed));
    }
    if (greenChanged)
    {
      NVS.setInt("lastGreen", lastGreen);
      debugPrintln(String(F("LED: lastGreen: ")) + String(lastGreen));
    }
    if (blueChanged)
    {
      NVS.setInt("lastBlue", lastBlue);
      debugPrintln(String(F("LED: lastBlue: ")) + String(lastBlue));
    }
    if (lightsOnChanged)
    {
      NVS.setInt("lastLightsOn", lastLightsOn ? 1 : 0);
      debugPrintln(lastLightsOn ? String(F("LED: lastOn: ON")) : String(F("LED: lastOn: OFF")));
    }

    mqttStateUpdate();

    NVS.commit();
  }
}

void ledsSetup()
{
  debugPrintln(String(F("LED: LED Count: ")) + String(ledsToUse));
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, atoi(ledsToUse)); // GRB ordering is assumed

  FastLED.setBrightness(128);
  fill_solid(leds, atoi(ledsToUse), CRGB::Red);
  FastLED.show();
  delay(250);
  fill_solid(leds, atoi(ledsToUse), CRGB::Green);
  FastLED.show();
  delay(250);
  fill_solid(leds, atoi(ledsToUse), CRGB::Blue);
  FastLED.show();
  delay(250);

  red = NVS.getInt("lastRed");
  green = NVS.getInt("lastGreen");
  blue = NVS.getInt("lastBlue");
  brightness = NVS.getInt("lastBrightness");
  lightsOn = NVS.getInt("lastLightsOn") == 1;

  debugPrintln(String(F("LED: Red: ")) + String(red));
  debugPrintln(String(F("LED: Green: ")) + String(green));
  debugPrintln(String(F("LED: Blue: ")) + String(blue));
  debugPrintln(String(F("LED: Brightness: ")) + String(brightness));
  debugPrintln(String(F("LED: LightsOn: ")) + String(lightsOn));

  lastLightsOn = !lastLightsOn;
  lastRed = lastGreen = lastBlue = lastBrightness = 0;
}

void mqttCallback(String &strTopic, String &strPayload)
{
  debugPrintln(String(F("MQTT IN: '")) + strTopic + "' : '" + strPayload + "'");

  if (strTopic == mqttTopicSet && strPayload == "")
  {
    mqttSensorUpdate(); // return status JSON via MQTT
  }
  else if (strTopic == mqttTopicSet && strPayload != "")
  {
    mqttParseJson(strPayload);
  }
  else if (strTopic == (mqttTopicSet + "/statusupdate"))
  {
    mqttSensorUpdate(); // return status JSON via MQTT
  }
  else if (strTopic == (mqttTopicSet + "/reboot"))
  {
    debugPrintln(F("MQTT: Rebooting device"));
    espReset();
  }
  else if (strTopic == (mqttTopicSet + "/factoryreset"))
  {
    configClearSaved();
  }
}

// MQTT connection and subscriptions
void mqttConnect()
{

  // static bool mqttFirstConnect = true; // For the first connection, we want to send an OFF/ON state to
  //                                      // trigger any automations, but skip that if we reconnect while
  //                                      // still running the sketch

  // MQTT topic string definitions
  mqttTopicSet = "fcrgb/" + String(fcrgbNode) + "/set";
  mqttTopicStatus = "fcrgb/" + String(fcrgbNode) + "/status";
  mqttTopicSensor = "fcrgb/" + String(fcrgbNode) + "/sensor";

  const String mqttCommandSubscription = mqttTopicSet + "/#";

  // Loop until we're reconnected to MQTT
  while (!mqttClient.connected())
  {
    // Create a reconnect counter
    static uint8_t mqttReconnectCount = 0;

    // Generate an MQTT client ID as fcrgbNode + our MAC address
    mqttClientId = String(fcrgbNode) + "-" + String(espMac[0], HEX) + String(espMac[1], HEX) + String(espMac[2], HEX) + String(espMac[3], HEX) + String(espMac[4], HEX) + String(espMac[5], HEX);
    debugPrintln(String(F("MQTT: Attempting connection to broker ")) + String(mqttServer) + " as clientID " + mqttClientId);

    // Set keepAlive, cleanSession, timeout
    mqttClient.setOptions(30, true, 5000);

    // TODO: What is this?
    // declare LWT
    //mqttClient.setWill(mqttTopicStatus.c_str(), "OFF");

    // Attempt to connect to broker, setting last will and testament
    if (mqttClient.connect(mqttClientId.c_str(), mqttUser, mqttPassword))
    {
      // Subscribe to our topics
      if (mqttClient.subscribe(mqttTopicSet))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttTopicSet);
      }
      if (mqttClient.subscribe(mqttTopicStatus))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttTopicStatus);
      }
      if (mqttClient.subscribe(mqttTopicSensor))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttTopicSensor);
      }
      if (mqttClient.subscribe(mqttCommandSubscription))
      {
        debugPrintln(String(F("MQTT: subscribed to ")) + mqttCommandSubscription);
      }

      // TODO: Handle this with json...
      // if (mqttFirstConnect)
      // { // Force any subscribed clients to toggle OFF/ON when we first connect to
      //   // make sure we get a full panel refresh at power on.  Sending OFF,
      //   // "ON" will be sent by the mqttStatusTopic subscription action.
      //   debugPrintln(String(F("MQTT: binary_sensor state: [")) + mqttTopicStatus + "] : [OFF]");
      //   mqttClient.publish(mqttTopicStatus, "OFF", true, 1);
      //   mqttFirstConnect = false;
      // }
      // else
      // {
      //   debugPrintln(String(F("MQTT: binary_sensor state: [")) + mqttTopicStatus + "] : " + lightsOn ? "[ON]" : "[OFF]");
      //   mqttClient.publish(mqttTopicStatus, lightsOn ? "ON" : "OFF", true, 1);
      // }

      mqttReconnectCount = 0;

      // Update panel with MQTT status
      debugPrintln(F("MQTT: connected"));
    }
    else
    { // Retry until we give up and restart after connectTimeout seconds
      mqttReconnectCount++;
      if (mqttReconnectCount > ((connectTimeout / 10) - 1))
      {
        debugPrintln(String(F("MQTT connection attempt ")) + String(mqttReconnectCount) + String(F(" failed with rc ")) + String(mqttClient.returnCode()) + String(F(".  Restarting device.")));
        espReset();
      }
      debugPrintln(String(F("MQTT connection attempt ")) + String(mqttReconnectCount) + String(F(" failed with rc ")) + String(mqttClient.returnCode()) + String(F(".  Trying again in 30 seconds.")));
      unsigned long mqttReconnectTimer = millis(); // record current time for our timeout
      while ((millis() - mqttReconnectTimer) < 30000)
      { // Handle HTTP and OTA while we're waiting 30sec for MQTT to reconnect
        webServer.handleClient();
        ArduinoOTA.handle();
        delay(10);
      }
    }
  }
}

// Parse an incoming JSON array
void mqttParseJson(String &strPayload)
{
  if (strPayload.endsWith(",]"))
  { // Trailing null array elements are an artifact of older Home Assistant automations and need to
    // be removed before parsing by ArduinoJSON 6+
    strPayload.remove(strPayload.length() - 2, 2);
    strPayload.concat("]");
  }
  DynamicJsonDocument fcrgbCommand(mqttMaxPacketSize + 1024);
  DeserializationError jsonError = deserializeJson(fcrgbCommand, strPayload);
  if (jsonError)
  { // Couldn't parse incoming JSON command
    debugPrintln(String(F("MQTT: [ERROR] Failed to parse incoming JSON command with error: ")) + String(jsonError.c_str()));
  }
  else
  {
    // state: ON/OFF
    const char *state = fcrgbCommand["state"];
    if (state)
    {
      debugPrintln(String(F("MQTT Parse: state")));
      lightsOn = strcmp(state, "ON") == 0;
    }

    // color: {r: 0 - 255, g: 0 - 255, b: 0 - 255}
    auto color = fcrgbCommand["color"];
    if (color)
    {
      debugPrintln(String(F("MQTT Parse: color")));
      red = color["r"];
      green = color["g"];
      blue = color["b"];
    }

    const char *e = fcrgbCommand["effect"];
    if (e)
    {
      if (strcmp(e, "reset") == 0)
      {
        debugPrintln(String(F("MQTT Parse: effect none")));
        effect = "";
        lastBrightness = 0;
        lastRed = 0;
        lastGreen = 0;
        lastBrightness = 0;
        lastLightsOn = !lightsOn;
      }
      else
      {
        debugPrintln(String(F("MQTT Parse: effect ")) + String(e));
        effect = String(e);
      }
    }

    // brightness: 1 - 255
    uint8_t bright = fcrgbCommand["brightness"];
    if (bright)
    {
      debugPrintln(String(F("MQTT Parse: brightness")));
      if (effect.isEmpty())
      {
        brightness = bright;
      }
      else
      {
        FastLED.setBrightness(bright);
      }
    }

    // TODO transition:
  }
}

// Handle incoming commands from MQTT
void mqttSetup()
{
  mqttClient.begin(mqttServer, atoi(mqttPort), wifiMQTTClient); // Create MQTT service object
  mqttClient.onMessage(mqttCallback);                           // Setup MQTT callback function
  mqttConnect();                                                // Connect to MQTT
}

// Periodically publish a JSON string indicating system status
void mqttSensorUpdate()
{
  String mqttStatusPayload = "{";
  mqttStatusPayload += String(F("\"status\":\"available\","));
  mqttStatusPayload += String(F("\"espVersion\":")) + String(fcrgbVersion) + String(F(","));
  mqttStatusPayload += String(F("\"espUptime\":")) + String(long(millis() / 1000)) + String(F(","));
  mqttStatusPayload += String(F("\"signalStrength\":")) + String(WiFi.RSSI()) + String(F(","));
  mqttStatusPayload += String(F("\"fcrgbIP\":\"")) + WiFi.localIP().toString() + String(F("\","));
  mqttStatusPayload += String(F("\"heapFree\":")) + String(ESP.getFreeHeap()) + String(F(","));
#ifdef ESP32
  // TODO what are the ESP32 APIs for heap fragmentation and core version?
#else
  mqttStatusPayload += String(F("\"heapFragmentation\":")) + String(ESP.getHeapFragmentation()) + String(F(","));
  mqttStatusPayload += String(F("\"espCore\":\"")) + String(ESP.getCoreVersion()) + String(F("\""));
#endif
  mqttStatusPayload += "}";

  mqttClient.publish(mqttTopicSensor, mqttStatusPayload, true, 1);
  debugPrintln(String(F("MQTT: status update: ")) + String(mqttStatusPayload));
  // debugPrintln(String(F("MQTT: binary_sensor state: [")) + mqttTopicStatus + "] : " + lightsOn ? "[ON]" : "[OFF]");
}

void mqttStateUpdate()
{
  debugPrintln(String(F("MQTT: state update")));
  StaticJsonDocument<mqttMaxPacketSize + 1024> doc;
  JsonObject root = doc.to<JsonObject>();
  root["state"] = lightsOn ? "ON" : "OFF";
  root["brightness"] = brightness;
  JsonObject color = root.createNestedObject("color");
  color["r"] = red;
  color["g"] = green;
  color["b"] = blue;
  if (!effect.isEmpty())
  {
    root["effect"] = effect;
  }

  // TODO: transition

  String state;
  serializeJsonPretty(root, state);
  mqttClient.publish(mqttTopicStatus, state, true, 1);
}

// http://fcrgb01/configReset
void webHandleConfigReset()
{
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!webServer.authenticate(configUser, configPassword))
    {
      return webServer.requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /configReset page to client connected from: ")) + webServer.client().remoteIP().toString());
  String httpMessage = FPSTR(WM_HTTP_HEAD_START);
  httpMessage.replace("{v}", String(fcrgbNode));
  httpMessage += FPSTR(WM_HTTP_SCRIPT);
  httpMessage += FPSTR(WM_HTTP_STYLE);
  httpMessage += String(FCRGB_STYLE);
  httpMessage += FPSTR(WM_HTTP_HEAD_END);

  if (webServer.arg("confirm") == "yes")
  { // User has confirmed, so reset everything
    httpMessage += String(F("<h1>"));
    httpMessage += String(fcrgbNode);
    httpMessage += String(F("</h1><b>Resetting all saved settings and restarting device into WiFi AP mode</b>"));
    httpMessage += FPSTR(WM_HTTP_END);
    webServer.send(200, "text/html", httpMessage);
    delay(1000);
    configClearSaved();
  }
  else
  {
    httpMessage += String(F("<h1>Warning</h1><b>This process will reset all settings to the default values and restart the device.  You may need to connect to the WiFi AP displayed on the panel to re-configure the device before accessing it again."));
    httpMessage += String(F("<br/><hr><br/><form method='get' action='configReset'>"));
    httpMessage += String(F("<br/><br/><button type='submit' name='confirm' value='yes'>reset all settings</button></form>"));
    httpMessage += String(F("<br/><hr><br/><form method='get' action='/'>"));
    httpMessage += String(F("<button type='submit'>return home</button></form>"));
    httpMessage += FPSTR(WM_HTTP_END);
    webServer.send(200, "text/html", httpMessage);
  }
}

// http://fcrgb01/configSave
void webHandleConfigSave()
{
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!webServer.authenticate(configUser, configPassword))
    {
      return webServer.requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /configSave page to client connected from: ")) + webServer.client().remoteIP().toString());
  String httpMessage = FPSTR(WM_HTTP_HEAD_START);
  httpMessage.replace("{v}", String(fcrgbNode));
  httpMessage += FPSTR(WM_HTTP_SCRIPT);
  httpMessage += FPSTR(WM_HTTP_STYLE);
  httpMessage += String(FCRGB_STYLE);

  bool shouldSaveWifi = false;
  // Check required values
  if (webServer.arg("wifiSSID") != "" && webServer.arg("wifiSSID") != String(WiFi.SSID()))
  { // Handle WiFi update
    shouldSaveConfig = true;
    shouldSaveWifi = true;
    webServer.arg("wifiSSID").toCharArray(wifiSSID, 32);
    if (webServer.arg("wifiPass") != String("********"))
    {
      webServer.arg("wifiPass").toCharArray(wifiPass, 64);
    }
  }
  if (webServer.arg("mqttServer") != "" && webServer.arg("mqttServer") != String(mqttServer))
  { // Handle mqttServer
    shouldSaveConfig = true;
    webServer.arg("mqttServer").toCharArray(mqttServer, 64);
  }
  if (webServer.arg("mqttPort") != "" && webServer.arg("mqttPort") != String(mqttPort))
  { // Handle mqttPort
    shouldSaveConfig = true;
    webServer.arg("mqttPort").toCharArray(mqttPort, 6);
  }
  if (webServer.arg("fcrgbNode") != "" && webServer.arg("fcrgbNode") != String(fcrgbNode))
  { // Handle fcrgbNode
    shouldSaveConfig = true;
    String lowerHaspNode = webServer.arg("fcrgbNode");
    lowerHaspNode.toLowerCase();
    lowerHaspNode.toCharArray(fcrgbNode, 16);
  }
  // Check optional values
  if (webServer.arg("mqttUser") != String(mqttUser))
  { // Handle mqttUser
    shouldSaveConfig = true;
    webServer.arg("mqttUser").toCharArray(mqttUser, 32);
  }
  if (webServer.arg("mqttPassword") != String("********"))
  { // Handle mqttPassword
    shouldSaveConfig = true;
    webServer.arg("mqttPassword").toCharArray(mqttPassword, 32);
  }
  if (webServer.arg("configUser") != String(configUser))
  { // Handle configUser
    shouldSaveConfig = true;
    webServer.arg("configUser").toCharArray(configUser, 32);
  }
  if (webServer.arg("configPassword") != String("********"))
  { // Handle configPassword
    shouldSaveConfig = true;
    webServer.arg("configPassword").toCharArray(configPassword, 32);
  }
  if (webServer.arg("ledsToUse") != "" && webServer.arg("ledsToUse") != String(ledsToUse))
  { // Handle ledsToUse
    shouldSaveConfig = true;
    webServer.arg("ledsToUse").toCharArray(ledsToUse, 4);
  }

  if (shouldSaveConfig)
  { // Config updated, notify user and trigger write to SPIFFS
    httpMessage += String(F("<meta http-equiv='refresh' content='15;url=/' />"));
    httpMessage += FPSTR(WM_HTTP_HEAD_END);
    httpMessage += String(F("<h1>")) + String(fcrgbNode) + String(F("</h1>"));
    httpMessage += String(F("<br/>Saving updated configuration values and restarting device"));
    httpMessage += FPSTR(WM_HTTP_END);
    webServer.send(200, "text/html", httpMessage);

    configSave();
    if (shouldSaveWifi)
    {
      debugPrintln(String(F("CONFIG: Attempting connection to SSID: ")) + webServer.arg("wifiSSID"));
      espWifiSetup();
    }
    espReset();
  }
  else
  { // No change found, notify user and link back to config page
    httpMessage += String(F("<meta http-equiv='refresh' content='3;url=/' />"));
    httpMessage += FPSTR(WM_HTTP_HEAD_END);
    httpMessage += String(F("<h1>")) + String(fcrgbNode) + String(F("</h1>"));
    httpMessage += String(F("<br/>No changes found, returning to <a href='/'>home page</a>"));
    httpMessage += FPSTR(WM_HTTP_END);
    webServer.send(200, "text/html", httpMessage);
  }
}

// webServer 404
void webHandleNotFound()
{
  debugPrintln(String(F("HTTP: Sending 404 to client connected from: ")) + webServer.client().remoteIP().toString());
  String httpMessage = "File Not Found\n\n";
  httpMessage += "URI: ";
  httpMessage += webServer.uri();
  httpMessage += "\nMethod: ";
  httpMessage += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  httpMessage += "\nArguments: ";
  httpMessage += webServer.args();
  httpMessage += "\n";
  for (uint8_t i = 0; i < webServer.args(); i++)
  {
    httpMessage += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, "text/plain", httpMessage);
}

// http://fcrgb01/reboot
void webHandleReboot()
{
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!webServer.authenticate(configUser, configPassword))
    {
      return webServer.requestAuthentication();
    }
  }
  debugPrintln(String(F("HTTP: Sending /reboot page to client connected from: ")) + webServer.client().remoteIP().toString());
  String httpMessage = FPSTR(WM_HTTP_HEAD_START);
  httpMessage.replace("{v}", (String(fcrgbNode) + " FCRGB reboot"));
  httpMessage += FPSTR(WM_HTTP_SCRIPT);
  httpMessage += FPSTR(WM_HTTP_STYLE);
  httpMessage += String(FCRGB_STYLE);
  httpMessage += String(F("<meta http-equiv='refresh' content='10;url=/' />"));
  httpMessage += FPSTR(WM_HTTP_HEAD_END);
  httpMessage += String(F("<h1>")) + String(fcrgbNode) + String(F("</h1>"));
  httpMessage += String(F("<br/>Rebooting device"));
  httpMessage += FPSTR(WM_HTTP_END);
  webServer.send(200, "text/html", httpMessage);
  debugPrintln(F("RESET: Rebooting device"));
  espReset();
}

// http://fcrgb01/
void webHandleRoot()
{
  if (configPassword[0] != '\0')
  { //Request HTTP auth if configPassword is set
    if (!webServer.authenticate(configUser, configPassword))
    {
      return webServer.requestAuthentication();
    }
  }

  debugPrintln(String(F("HTTP: Sending root page to client connected from: ")) + webServer.client().remoteIP().toString());
  String httpMessage = FPSTR(WM_HTTP_HEAD_START);
  httpMessage.replace("{v}", String(fcrgbNode));
  httpMessage += FPSTR(WM_HTTP_SCRIPT);
  httpMessage += FPSTR(WM_HTTP_STYLE);
  httpMessage += String(FCRGB_STYLE);
  httpMessage += FPSTR(WM_HTTP_HEAD_END);
  httpMessage += String(F("<h1>"));
  httpMessage += String(fcrgbNode);
  httpMessage += String(F("</h1>"));

  httpMessage += String(F("<form method='POST' action='configSave'>"));
  httpMessage += String(F("<b>WiFi SSID</b> <i><small>(required)</small></i><input id='wifiSSID' required name='wifiSSID' maxlength=32 placeholder='WiFi SSID' value='")) + String(WiFi.SSID()) + "'>";
  httpMessage += String(F("<br/><b>WiFi Password</b> <i><small>(required)</small></i><input id='wifiPass' required name='wifiPass' type='password' maxlength=64 placeholder='WiFi Password' value='")) + String("********") + "'>";
  httpMessage += String(F("<br/><br/><b>FCRGB Node Name</b> <i><small>(required. lowercase letters, numbers, and _ only)</small></i><input id='fcrgbNode' required name='fcrgbNode' maxlength=15 placeholder='FCRGB Node Name' pattern='[a-z0-9_]*' value='")) + String(fcrgbNode) + "'>";
  httpMessage += String(F("<br/><b>Number of LEDs</b> <i><small>(required)</small></i><input id='ledsToUse' required name='ledsToUse' type='number' maxlength=3 placeholder='LED count' value='")) + String(ledsToUse) + "'>";
  httpMessage += String(F("<br/><br/><b>MQTT Broker</b> <i><small>(required)</small></i><input id='mqttServer' required name='mqttServer' maxlength=63 placeholder='mqttServer' value='")) + String(mqttServer) + "'>";
  httpMessage += String(F("<br/><b>MQTT Port</b> <i><small>(required)</small></i><input id='mqttPort' required name='mqttPort' type='number' maxlength=5 placeholder='mqttPort' value='")) + String(mqttPort) + "'>";
  httpMessage += String(F("<br/><b>MQTT User</b> <i><small>(optional)</small></i><input id='mqttUser' name='mqttUser' maxlength=31 placeholder='mqttUser' value='")) + String(mqttUser) + "'>";
  httpMessage += String(F("<br/><b>MQTT Password</b> <i><small>(optional)</small></i><input id='mqttPassword' name='mqttPassword' type='password' maxlength=31 placeholder='mqttPassword' value='"));
  if (strlen(mqttPassword) != 0)
  {
    httpMessage += String("********");
  }
  httpMessage += String(F("'><br/><br/><b>FCRGB Admin Username</b> <i><small>(optional)</small></i><input id='configUser' name='configUser' maxlength=31 placeholder='Admin User' value='")) + String(configUser) + "'>";
  httpMessage += String(F("<br/><b>FCRGB Admin Password</b> <i><small>(optional)</small></i><input id='configPassword' name='configPassword' type='password' maxlength=31 placeholder='Admin User Password' value='"));
  if (strlen(configPassword) != 0)
  {
    httpMessage += String("********");
  }

  httpMessage += String(F("'><br/><hr><button type='submit'>save settings</button></form>"));

  httpMessage += String(F("<hr><form method='get' action='reboot'>"));
  httpMessage += String(F("<button type='submit'>reboot device</button></form>"));

  httpMessage += String(F("<hr><form method='get' action='configReset'>"));
  httpMessage += String(F("<button type='submit'>factory reset settings</button></form>"));

  httpMessage += String(F("<hr><b>MQTT Status: </b>"));
  if (mqttClient.connected())
  { // Check MQTT connection
    httpMessage += String(F("Connected"));
  }
  else
  {
    httpMessage += String(F("<font color='red'><b>Disconnected</b></font>, return code: ")) + String(mqttClient.returnCode());
  }
  httpMessage += String(F("<br/><b>MQTT ClientID: </b>")) + String(mqttClientId);
  httpMessage += String(F("<br/><b>FCRGB Version: </b>")) + String(fcrgbVersion);
  httpMessage += String(F("<br/><b>CPU Frequency: </b>")) + String(ESP.getCpuFreqMHz()) + String(F("MHz"));
  httpMessage += String(F("<br/><b>Sketch Size: </b>")) + String(ESP.getSketchSize()) + String(F(" bytes"));
  httpMessage += String(F("<br/><b>Free Sketch Space: </b>")) + String(ESP.getFreeSketchSpace()) + String(F(" bytes"));
  httpMessage += String(F("<br/><b>Heap Free: </b>")) + String(ESP.getFreeHeap());
#ifdef ESP32
#else
  httpMessage += String(F("<br/><b>Heap Fragmentation: </b>")) + String(ESP.getHeapFragmentation());
  httpMessage += String(F("<br/><b>ESP core version: </b>")) + String(ESP.getCoreVersion());
#endif
  httpMessage += String(F("<br/><b>IP Address: </b>")) + String(WiFi.localIP().toString());
  httpMessage += String(F("<br/><b>Signal Strength: </b>")) + String(WiFi.RSSI());
  httpMessage += String(F("<br/><b>Uptime: </b>")) + String(long(millis() / 1000));
#ifdef ESP32
  httpMessage += String(F("<br/><b>Last reset: </b>")) + String(rtc_get_reset_reason(0));
#else
  httpMessage += String(F("<br/><b>Last reset: </b>")) + String(ESP.getResetInfo());
#endif

  httpMessage += FPSTR(WM_HTTP_END);
  webServer.send(200, "text/html", httpMessage);
}

void webServerSetup()
{
  webServer.on("/", webHandleRoot);
  webServer.on("/configSave", webHandleConfigSave);
  webServer.on("/configReset", webHandleConfigReset);
  webServer.on("/reboot", webHandleReboot);
  webServer.onNotFound(webHandleNotFound);
  webServer.begin();

  debugPrintln(String(F("HTTP: Server started @ http://")) + WiFi.localIP().toString());
}
