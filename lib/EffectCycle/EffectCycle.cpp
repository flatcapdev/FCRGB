#include "EffectCycle.h"

int cycleDelay = 100;
uint8_t cycleH;
unsigned long cyclePreviousMillis;

void handleCycle(struct CRGB * leds, int numToFill)
{
  unsigned long currentMillis = millis();
  if (currentMillis - cyclePreviousMillis > cycleDelay)
  {
    cyclePreviousMillis = currentMillis;
    fill_solid(leds, numToFill, CHSV(cycleH, 255, 255));
    cycleH += 1;
    FastLED.show();
  }
}
