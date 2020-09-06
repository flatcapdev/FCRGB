#include "EffectFire.h"

  //gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
  // This first palette is the basic 'black body radiation' colors,
  // which run from black to red to bright yellow to white.
  // gPal = HeatColors_p;

  // These are other ways to set up the color palette for the 'fire'.
  // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
  CRGBPalette16 gPal = CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::Orange, CRGB::Yellow);

  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  // gPal = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

  // Third, here's a simpler, three-step gradient, from black to red to white
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);


void handleFire(struct CRGB * leds, int numToFill)
{
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(2147483647));
  int ledCnt = numToFill;

  // Array of temperature readings at each simulation cell
  static byte heat[MAX_NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for (int i = 0; i < ledCnt; i++)
  {
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / ledCnt) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (int k = ledCnt - 3; k > 0; k--)
  {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING)
  {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (int j = 0; j < ledCnt; j++)
  {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8(heat[j], 240);
    leds[j] = ColorFromPalette(gPal, colorindex);
  }

  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}