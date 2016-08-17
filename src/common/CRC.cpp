#include "CRC.h"

uint8_t CRCItem(uint8_t msg)
{
   uint8_t CRCPoly = 0x89;  // the value of our CRC-7 polynomial

   uint8_t item = (msg & 0x80) ? msg ^ CRCPoly : msg;
   for (int j = 1; j < 8; ++j) 
   {
      item <<= 1;
      if (item & 0x80)
         item ^= CRCPoly;
   }
  
   return item;
}
 
// adds a message byte to the current CRC-7 to get a the new CRC-7
uint8_t CRCAdd(uint8_t CRC, uint8_t message_byte)
{
   return CRCItem((CRC << 1) ^ message_byte);
}
 
// returns the CRC-7 for a message of "length" bytes
uint8_t psvcd::CRC_7(const uint8_t* data, int length)
{
  int i;
  uint8_t CRC = 0;

  for (i = 0; i < length; ++i)
    CRC = CRCAdd(CRC, data[i]);

  return (CRC << 1) | 1;
}

// Calculate CRC16 CCITT
// It's a 16 bit CRC with polynomial x^16 + x^12 + x^5 + 1
// input:
//   crcIn - the CRC before (0 for rist step)
//   data - byte for CRC calculation
// return: the CRC16 value
uint16_t CRC_16_one(uint16_t crcIn, uint8_t data) 
{
	crcIn  = (uint8_t)(crcIn >> 8)|(crcIn << 8);
	crcIn ^=  data;
	crcIn ^= (uint8_t)(crcIn & 0xff) >> 4;
	crcIn ^= (crcIn << 8) << 4;
	crcIn ^= ((crcIn & 0xff) << 4) << 1;

	return crcIn;
}

// Calculate CRC16 CCITT value of the buffer
// input:
//   pBuf - pointer to the buffer
//   len - length of the buffer
// return: the CRC16 value
uint16_t psvcd::CRC_16(const uint8_t* pBuf, uint16_t len) 
{
	uint16_t crc = 0;

	while (len--) 
      crc = CRC_16_one(crc,*pBuf++);

	return crc;
}