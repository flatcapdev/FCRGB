#include "EffectCandle.h"

int candleDely = 0;
uint8_t candleBrightness = 0;
unsigned long candlePreviousMillis;

void handleCandle(struct CRGB *leds, int numToFill)
{
    handleCandle(leds, numToFill, CRGB(255, 96, 0));
}

void handleCandle(struct CRGB *leds, int numToFill, CRGB color)
{
    unsigned long currentMillis = millis();
    if (currentMillis - candlePreviousMillis > candleDely)
    {
        // candlePreviousMillis = currentMillis;
        fill_solid(leds, numToFill, color);
        uint8_t newBrightness = random(32, 128);
        uint8_t fadeDelay = random(3, 7);
        for (uint8_t b = candleBrightness; b != newBrightness; newBrightness > candleBrightness ? b++ : b--)
        {
            FastLED.setBrightness(b);
            FastLED.show();
            delay(fadeDelay);
        }
        candleBrightness = newBrightness;

        candlePreviousMillis = millis();
        candleDely = random(100, 1000);
    }
}