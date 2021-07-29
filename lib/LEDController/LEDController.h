#pragma once

#include <FastLED.h>
#include "../../include/defines.h"

class LEDController
{
public:
    LEDController();
    void setBrightness(uint8_t val);
    void update();
    void init();

    uint8_t hue;

    struct {
        bool areLedStripsEnabled;
    } config;

private:
    void ledInit();
    void spinner();

    CRGB _ledsOfRing[NUM_LEDS_RING];
    CRGB _ledsOfStrip[NUM_LEDS_STRIP];
    bool _isLedInit;
    bool _isLightLevelSet;
};