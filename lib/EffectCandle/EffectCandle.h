#ifndef __EffectCandle_h
#define __EffectCandle_h

#include <Arduino.h>
#include<FastLED.h>

void handleCandle(struct CRGB *leds, int numToFill);
void handleCandle(struct CRGB *leds, int numToFill, CRGB color);

#endif