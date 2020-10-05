#include "EffectCylon.h"

int cylonDir = 1;
static uint8_t cylonHue = 0;
int cylonPos = 0;
int cylonDelay = 25;
unsigned long cylonPreviousMillis;

void fadeall(struct CRGB *leds, int numToFill)
{
    for (int i = 0; i < numToFill; i++)
    {
        leds[i].nscale8(250);
    }
}
void handleCylon(struct CRGB *leds, int numToFill, bool rainbow, CRGB color)
{
    unsigned long currentMillis = millis();
    if (currentMillis - cylonPreviousMillis > cylonDelay)
    {
        cylonPreviousMillis = currentMillis;

        if (rainbow)
        {
            leds[cylonPos] = CHSV(cylonHue++, 255, 255);
        }
        else
        {
            leds[cylonPos] = color;
        }
        FastLED.show();
        fadeall(leds, numToFill);

        cylonPos += cylonDir;
        if (cylonPos == numToFill || cylonPos == -1)
        {
            cylonDir *= -1;
            cylonPos += cylonDir;
        }
    }
}

void handleCylon(struct CRGB *leds, int numToFill)
{
    handleCylon(leds, numToFill, true, CRGB::Red);
}

void handleCylon(struct CRGB *leds, int numToFill, CRGB color)
{
    handleCylon(leds, numToFill, false, color);
}