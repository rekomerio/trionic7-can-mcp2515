#pragma once

#include <Arduino.h>
#include "communication.h"

void readCanBus();
uint8_t scaleBrightness(uint16_t val, uint16_t minimum, uint16_t maximum);
void readCanBus();
void steeringWheelActions(STEERING_WHEEL action);
void sidActions(SID_BUTTON action);
void lightActions(const uint8_t *data);
void vehicleActions(const uint8_t *data);
uint8_t getHighBit(const uint8_t value);
uint16_t combineBytes(uint8_t byte1, uint8_t byte2);
uint32_t elapsed(uint32_t time);