#pragma once

#include <stdint.h>

uint8_t inByte(uint16_t port);
void outByte(uint16_t port, uint8_t data);
uint32_t inLong(uint16_t port);
void outLong(uint16_t port, uint32_t data);