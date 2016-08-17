#pragma once

#include <stdint.h>

namespace psvcd {

//CRC_7 - is a modified version of this algorithm
//https://github.com/hazelnusse/crc7/blob/master/crc7.cc

uint8_t CRC_7(const uint8_t* data, int length);

//CRC_16 - is taken from this algorithm
//https://github.com/LonelyWolf/stm32/blob/master/stm32l-dosfs/sdcard.c
uint16_t CRC_16(const uint8_t* pBuf, uint16_t len);

};