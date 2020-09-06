#ifndef __EffectRWBGlitter_h
#define __EffectRWBGlitter_h

#include <Arduino.h>
#include <FastLED.h>

void handleRWBGlitter(struct CRGB * leds, int numToFill);
void handleRWBGlitter(struct CRGB * leds, int numToFill, bool christmas);

#endif