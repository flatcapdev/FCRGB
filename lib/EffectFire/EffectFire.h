#ifndef __EffectFire_h
#define __EffectFire_h

#include <Arduino.h>
#include <FastLED.h>

// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100
#define COOLING 55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120
#define FRAMES_PER_SECOND 20

// THIS NEEDS TO MATCH WHAT IS DEFINED IN THE MAIN PROJECT!
#define MAX_NUM_LEDS 601

void handleFire(struct CRGB * leds, int numToFill);

#endif