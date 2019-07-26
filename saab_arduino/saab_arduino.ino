#include <FastLED.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

//BLUETOOTH PINS turn on bluetooth receiver
#define BLUETOOTH_PIN0 9
#define BLUETOOTH_PIN1 10
#define TRANSISTOR_PIN 2 //TRANSISTOR PIN turns on radio telephone channel
#define BUTTON_PIN 7
#define LED_PIN 3

#define NUM_LEDS 12

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BLUETOOTH_PIN0, OUTPUT);
  pinMode(BLUETOOTH_PIN1, OUTPUT);
  pinMode(TRANSISTOR_PIN, OUTPUT);
  delay(100);

  // Spinning animation at startup - 10 complete rounds
  for (uint16_t i = 0; i < NUM_LEDS * 10; i++) {
    uint16_t j = i + (NUM_LEDS / 2); // Set second LED half a round further
    leds[i % NUM_LEDS] = CHSV(220, 255, 255);
    leds[j % NUM_LEDS] = CHSV(180, 255, 255);
    FastLED.show();
    FastLED.clear();
    delay(50);
  }
}

uint8_t mode = 0;

void loop() {
  if (buttonPressed()) {
    mode++;
    mode %= 2;
  }

  switch (mode) {
    case 0:
      spinner(100, 50);
      bluetooth(false);
      break;

    case 1:
      spinner(220, 60);
      bluetooth(true);
      break;

    default:
      FastLED.clear();
      FastLED.show();
      delay(500);
      break;
  }
}
/**
  *  Check is the button being pressed
  *  @returns {bool}
  */
bool buttonPressed() {
  if (digitalRead(BUTTON_PIN)) {
    return true;
  }
  return false;
}
/*
   Turn bluetooth on or off
   @param on {bool} - is bluetooth turned on (true) or off (false)
*/
void bluetooth(bool on) {
  digitalWrite(BLUETOOTH_PIN0, on);
  digitalWrite(BLUETOOTH_PIN1, on);
  digitalWrite(TRANSISTOR_PIN, on);
}
/**
   Spinning LED animation with trailing tail
   @param hue        {uint8_t} - CHSV color
   @param brightness {uint8_t} - brightness of LED's
*/
void spinner(uint8_t hue, uint8_t brightness) {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, brightness);
    FastLED.show();
    fadeToBlackBy(leds, NUM_LEDS, brightness / (NUM_LEDS / 4));
    delay(85);
  }
}
