# FCRGB
Control a strip of RGB LEDs with an ESP32 (and at some point, an ESP8266) using [Home Assistant](https://www.home-assistant.io/)

## How it started
This project started with the desire to control some RGB LEDs for some "above the cabinet" accent lights in our kitchen. Of course, I could have bought an off-the-shelf solution and been done with it.  But where's the fun in that?

## Project goals
1) Use a microcontroler (such as an ESP32 or ESP8266 because they both have built in wifi) to control the lights.  I chose the ESP32 because I had them on hand.
   1) [x] Support ESP32
   1) [ ] Support ESP8266
1) [x] Use a captive portal to set up wifi so I don't have to worry about checking wifi creds into my repo.
1) Implement OTA firmware update so I don't have to have physical access to the mc when things change
   1) [x] Arduino OTA
   1) [ ] Web based OTA
1) Be controllable via home automation software so events can be scheduled.  Initially I started with support for [Mozilla Webthings](https://iot.mozilla.org/) but sadly, ran into limitations with the rules engine.  I'm sure it will get there, but for now, it was just too limiting.  Currently this project interacts with [Home Assistant](https://www.home-assistant.io/), although it could work with any platform that supports MQTT.
   1) [x] Basic Control
   1) [ ] Auto Discovery

## Inspiration
The general project structure took a lot of cues from the [HASwitchPlate](https://github.com/aderusha/HASwitchPlate) project.  It had a lot of the features I wanted to use.  The two places I deviated was I went with the ESP32 initially where HASwitchPlate uses an ESP8266.  The other is that I went with a native [PlatformIO](https://platformio.org/) project (although, HASwitchPlate can be used in PlatformIO).

More to come...
