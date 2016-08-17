#include <windows.h>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <chrono>
#include <thread>

#include <ftd2xx.h>

#include <common/CardRoutines.h>
#include <common/CRC.h>

#include "SDCommands.h"
#include "SDTypes.h"

bool psvcd::CMD0_SD(FT_HANDLE ftHandle)
{
   std::cout << "CMD0_SD" << std::endl;
   BYTE cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};

   uint8_t crc7 = psvcd::CRC_7(cmd, 5);

   if(crc7 != cmd[5])
   {
      std::cout << "CMD0_SD invalid CRC-7" << std::endl;
      return false;
   }

   psvcd::SendCMD_Hold(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x08);
   return true;
}

bool psvcd::CMD8_SD(FT_HANDLE ftHandle)
{
   std::cout << "CMD8_SD" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_ReadData(ftHandle, 0x48, 0x00, 0x00, 0x01, 0xAA, 0x87, 0x04, resp, 511 + 2);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSDStatus(resp);

      //validate

      if(resp.front() != 0x08)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp[5];

      std::cout << "valid: " << valid << std::endl;
      
      //TODO: need to check data packet here also
      //TODO: need to check card state by parsing responce flags

      return valid;
   }
   else
   {
      std::cout << "CMD8_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD55_SD(FT_HANDLE ftHandle)
{
   std::cout << "CMD55_SD" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, 0x77, 0x00, 0x00, 0x00, 0x00, 0x65, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSDStatus(resp);

      //validate

      if(resp.front() != 0x37)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state by parsing responce flags
      
      return valid;      
   }
   else
   {
      std::cout << "CMD55_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::ACMD41_SD(FT_HANDLE ftHandle, std::vector<BYTE>& resp)
{
   std::cout << "ACMD41_SD" << std::endl;

   psvcd::SendCMD_ReadResp(ftHandle, 0x69, 0x00, 0xFF, 0x80, 0x00, 0x85, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      if(resp.front() != 0x29)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state by parsing responce flags
      
      return valid;
   }
   else
   {
      std::cout << "ACMD41_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD55_ACMD41_SD(FT_HANDLE ftHandle)
{
   std::cout << "CMD55_ACMD41_SD" << std::endl;

   std::vector<BYTE> resp;

   while(true)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      //init
      resp.clear();

      if(!CMD55_SD(ftHandle))
         return false;

      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      //TODO: send first ACMD41 differently
      //in reality - first time ACMD41 should be issued with zero arg to understand what voltage is supported
      //I have just hardcoded it for now

      //After that - ACMD41 should be sent with arg NOT zero or card will never exit busy state

      if(!ACMD41_SD(ftHandle, resp))
         return false;

      if(resp.size() < 5)
         continue;

      if(resp[0] == 0x3F &&
         resp[2] == 0xFF &&
         resp[3] == 0x80 &&
         resp[4] == 0x00 &&
         resp[5] == 0xFF)
      {
         if(resp[1] == 0x00)
         {
            std::cout << "busy" << std::endl;
            continue;
         }
         else if(resp[1] == 0x80)
         {
            std::cout << "done" << std::endl;
            break;
         }
         else
         {
            std::cout << "unknown" << std::endl;
            break;
         }
      }
   }

   if(resp.size())
   {
      psvcd::PrintResp(resp);
      return true;
   }
   else
   {
      std::cout << "CMD55_ACMD41_SD no response" << std::endl;
      return false;
   }
}

//TODO: need to check card state with CMD13 and calculate is_valid since this command responce is not R3
bool psvcd::CMD2_SD(FT_HANDLE ftHandle)
{
   std::cout << "CMD2" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, 0x42, 0x00, 0x00, 0x00, 0x00, 0x4D, 15, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      //validate

      if(resp.size() < 17)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      uint8_t crc7 = CRC_7(resp.data(), 16);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      return valid;
   }
   else
   {
      std::cout << "CMD2_SD no response" << std::endl;
      return false;
   }
}

//TODO: need to check card state with CMD13 and calculate is_valid since this command responce is not R3
bool psvcd::CMD3_SD(FT_HANDLE ftHandle, WORD& RCA)
{
   std::cout << "CMD3" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, 0x43, 0x00, 0x00, 0x00, 0x00, 0x21, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSD_CMD3Status(resp);

      RCA = (resp[1] << 8) | resp[2];

      return true;
   }
   else
   {
      std::cout << "CMD3_SD no response" << std::endl;
      return false;
   }
}

//TODO: need to check card state with CMD13 and calculate is_valid since this command responce is not R3
bool psvcd::CMD10_SD(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD10_SD" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x4A, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 15, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      //validate

      if(resp.size() < 17)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      uint8_t crc7 = CRC_7(resp.data(), 16);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      return valid;
   }
   else
   {
      std::cout << "CMD10_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD13_SD(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD13_SD" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x4D, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 4, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      //validate

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSDStatus(resp);

      if(resp.front() != 0x0D)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      return valid;
   }
   else
   {
      std::cout << "CMD13_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD7_SD(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD7_SD" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x47, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   //TODO: does this really require polling DAT0 line - I need to wire SD card to board to check again
   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_Poll(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 4, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      //validate

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSDStatus(resp);

      if(resp.front() != 0x07)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state by parsing responce flags

      return valid;
   }
   else
   {
      std::cout << "CMD7_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD7_SD(FT_HANDLE ftHandle)
{
   std::cout << "CMD7_SD" << std::endl;

   BYTE cmd[] = {0x47, 0x00, 0x00, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   //TODO: does this really require polling DAT0 line - I need to wire SD card to board to check again
   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_Poll(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 4, resp); //2000

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      //validate

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSDStatus(resp);
      
      if(resp.front() != 0x07)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state by parsing responce flags

      return valid;
   }
   else
   {
      std::cout << "CMD7_SD no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD17_SD(FT_HANDLE ftHandle, BYTE A3, BYTE A2, BYTE A1, BYTE A0)
{
   std::cout << "CMD17_SD" << std::endl;

   BYTE cmd[] = {0x51, A3, A2, A1, A0, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_ReadData(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp, 511 + 2);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintSDStatus(resp);

      //validate

      if(resp.front() != 0x11)
         return false;

      uint8_t crc7 = CRC_7(resp.data(), 5);
      bool valid = crc7 == resp[5];

      std::cout << "valid: " << valid << std::endl;
      
      //TODO: need to check data packet here also
      //TODO: need to check card state by parsing responce flags

      return valid;
   }
   else
   {
      std::cout << "CMD17_SD no response" << std::endl;
      return false;
   }
}