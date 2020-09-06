#ifndef __EffectCylon_h
#define __EffectCylon_h

#include <Arduino.h>
#include <FastLED.h>

void handleCylon(struct CRGB * leds, int numToFill);
void handleCylon(struct CRGB * leds, int numToFill, bool rainbow);

#endif