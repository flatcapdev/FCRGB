#ifndef __EffectCandle_h
#define __EffectCandle_h

#include <Arduino.h>
#include<FastLED.h>

void handleCandle(struct CRGB *leds, int numToFill, uint8_t maxBrightness);
void handleCandle(struct CRGB *leds, int numToFill, uint8_t maxBrightness, CRGB color);

#endif