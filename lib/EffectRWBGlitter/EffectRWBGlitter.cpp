#include "EffectRWBGlitter.h"

// THIS MUST MATCH the max leds defined in the main project
uint8_t data[601];

CRGBPalette16 gPaletteRWB(
    CRGB::Black, CRGB::Black,
    CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red,
    CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
    CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue,
    CRGB::Black, CRGB::Black);

// CRGBPalette16 gPaletteChristmas(
//     CRGB::Black,
//     CRGB::Red,
//     CRGB::Black, CRGB::Black,
//     CRGB::Yellow,
//     CRGB::Black, CRGB::Black,
//     CRGB::Green,
//     CRGB::Black, CRGB::Black,
//     CRGB::Orange,
//     CRGB::Black, CRGB::Black,
//     CRGB::Blue,
//     CRGB::Black, CRGB::Black);

CRGBPalette16 gPaletteChristmas(
    CRGB::Black, CRGB::Black, CRGB::Black,
    CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red,
    CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black, CRGB::Black,
    CRGB::Green, CRGB::Green, CRGB::Green);
#define C9_Red 0xB80400
#define C9_Orange 0x902C02
#define C9_Green 0x046002
#define C9_Blue 0x070758
#define C9_White 0x606820

CRGBPalette16 gPaletteChristmas2(
    C9_Red, C9_Red, C9_Red,
    C9_Orange, C9_Orange, C9_Orange,
    C9_Green, C9_Green, C9_Green,
    C9_Blue, C9_Blue, C9_Blue, C9_Blue,
    C9_White, C9_White, C9_White);

void fill_data_array(struct CRGB *leds, int numToFill, bool christmas)
{
    static uint8_t startValue = 0;
    startValue = startValue - 2;

    uint8_t value = startValue;
    for (int i = 0; i < numToFill; i++)
    {
        data[i] = christmas ? value : triwave8(value); // convert value to an up-and-down wave
        value += christmas ? 14 : 7;
    }
}

void render_data_with_palette(struct CRGB *leds, int numToFill, bool christmas)
{
    for (int i = 0; i < numToFill; i++)
    {
        leds[i] = ColorFromPalette(christmas ? gPaletteChristmas2 : gPaletteRWB, data[i], 128, LINEARBLEND);
    }
}

void add_glitter(struct CRGB *leds, int numToFill)
{
    int chance_of_glitter = 5;  // percent of the time that we add glitter
    int number_of_glitters = 3; // number of glitter sparkles to add

    int r = random8(100);
    if (r < chance_of_glitter)
    {
        for (int j = 0; j < number_of_glitters; j++)
        {
            int pos = random16(numToFill);
            leds[pos] = CRGB::White; // very bright glitter
        }
    }
}

void handleRWBGlitter(struct CRGB *leds, int numToFill)
{
    handleRWBGlitter(leds, numToFill, false);
}

void handleRWBGlitter(struct CRGB *leds, int numToFill, bool christmas)
{
    fill_data_array(leds, numToFill, christmas);
    render_data_with_palette(leds, numToFill, christmas);
    add_glitter(leds, numToFill);

    FastLED.show();
    FastLED.delay(20);
}