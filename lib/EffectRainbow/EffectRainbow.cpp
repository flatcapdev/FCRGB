#include "EffectRainbow.h"

void handleRainbow(struct CRGB *leds, int numToFill)
{
    uint8_t hueRate = 255;                             // Effects cycle rate. (range >0 to 255)
    fill_rainbow(leds, numToFill, millis() / hueRate); // Start hue effected by time.
    FastLED.show();
}