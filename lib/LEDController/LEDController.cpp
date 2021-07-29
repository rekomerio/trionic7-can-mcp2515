#include "LEDController.h"

LEDController::LEDController()
{
    _isLightLevelSet = false;
    _isLedInit = false;
    config.areLedStripsEnabled = true;
    hue = HUE_GREEN;
}

void LEDController::init()
{
    FastLED.addLeds<NEOPIXEL, LED_RING_PIN>(_ledsOfRing, NUM_LEDS_RING);
    FastLED.addLeds<NEOPIXEL, LED_STRIP_PIN>(_ledsOfStrip, NUM_LEDS_STRIP);
}

void LEDController::setBrightness(uint8_t val)
{
    _isLightLevelSet = true;
    FastLED.setBrightness(val);
}

void LEDController::update()
{
    if (_isLightLevelSet)
    {
        if (_isLedInit)
        {
            EVERY_N_MILLISECONDS(85)
            {
                fill_solid(_ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, config.areLedStripsEnabled ? STRIP_BRIGHTNESS : 0));
                spinner();
                FastLED.show();
            }
        }
        else
        {
            EVERY_N_MILLISECONDS(50)
            {
                ledInit();
            }
        }
    }
}
/*
	Animate LED's on startup
*/
void LEDController::ledInit()
{
    static uint8_t i = 0;
    static uint8_t brightness = 0;
    constexpr uint8_t bIncrement = STRIP_BRIGHTNESS / NUM_LEDS_RING;
    uint8_t j = i + (NUM_LEDS_RING / 2);

    brightness += bIncrement;
    _ledsOfRing[i % NUM_LEDS_RING] = CHSV(220, 255, 255);
    _ledsOfRing[j % NUM_LEDS_RING] = CHSV(180, 255, 255);
    fill_solid(_ledsOfStrip, NUM_LEDS_STRIP, CHSV(hue, 255, brightness));
    FastLED.show();
    _ledsOfRing[i % NUM_LEDS_RING] = CRGB::Black;
    _ledsOfRing[j % NUM_LEDS_RING] = CRGB::Black;

    if (++i >= NUM_LEDS_RING)
    {
        _isLedInit = true;
    }
}
/*
   Spinning LED animation with trailing tail
*/
void LEDController::spinner()
{
    static uint8_t i = 0;

    fadeToBlackBy(_ledsOfRing, NUM_LEDS_RING, 85);
    _ledsOfRing[i++] = CHSV(hue, 255, 255);
    i %= NUM_LEDS_RING;
}

