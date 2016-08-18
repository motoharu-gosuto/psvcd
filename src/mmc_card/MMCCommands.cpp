#include <windows.h>
#include <iostream>
#include <stdint.h>
#include <chrono>
#include <thread>
#include <vector>

#include <ftd2xx.h>

#include <common/CRC.h>
#include <common/CardRoutines.h>

#include "MMCCommands.h"

bool psvcd::CMD0_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD0" << std::endl;
   BYTE cmd[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};

   uint8_t crc7 = psvcd::CRC_7(cmd, 5);

   if(crc7 != cmd[5])
   {
      std::cout << "CMD0 invalid CRC-7" << std::endl;
      return false;
   }

   psvcd::SendCMD_Hold(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x08);
   return true;
}

bool psvcd::CMD1_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD1" << std::endl;

   std::vector<BYTE> resp;

   while(true)
   {
      //init
      resp.clear();

      psvcd::SendCMD_ReadResp(ftHandle, 0x41, 0x40, 0xFF, 0x80, 0x00, 0x0B, 0x04, resp);

      if(resp.size() < 5)
         continue;

      if(resp[0] == 0x3F &&
         resp[2] == 0xFF &&
         resp[3] == 0x80 &&
         resp[4] == 0x80 &&
         resp[5] == 0xFF)
      {
         if(resp[1] == 0x00)
         {
            std::cout << "busy" << std::endl;
            continue;
         }
         else if(resp[1] == 0xC0)
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
      std::cout << "CMD1 no response" << std::endl;
      return false;
   }
}

//TODO: need to check card state with CMD13 and calculate is_valid since this command responce is not R1
bool psvcd::CMD2_MMC(FT_HANDLE ftHandle)
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

      //look at check byte
      if(resp.front() != 0x3F)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data() + 1, 15);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      return valid;
   }
   else
   {
      std::cout << "CMD2 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD3_MMC(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD3" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x43, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      //validate

      if(resp.front() != 0x03)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD3 no response" << std::endl;
      return false;
   }
}

//TODO: need to check card state with CMD13 and calculate is_valid since this command responce is not R1
bool psvcd::CMD9_MMC(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD9" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x49, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 15 , resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 17)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      //validate

      //look at check byte
      if(resp.front() != 0x3F)
         return false;
      
      uint8_t crc7 = psvcd::CRC_7(resp.data() + 1, 15);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      return valid;
   }
   else
   {
      std::cout << "CMD9 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD7_MMC(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD7" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x47, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      //validate

      if(resp.front() != 0x07)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD7 no response" << std::endl;
      return false;
   }
}

//TODO: In some cases CMD7 may require DAT0 busy polling - need to figure out this since sometimes CMD7 fails
//deselection does not have any responce - so here is special command that does not check it
bool psvcd::CMD7_MMC_Deselect(FT_HANDLE ftHandle)
{
   std::cout << "CMD7" << std::endl;

   BYTE cmd[] = {0x47, 0x00, 0x00, 0x00, 0x00, 0x83};

   uint8_t crc7 = psvcd::CRC_7(cmd, 5);

   if(crc7 != cmd[5])
   {
      std::cout << "CMD7 invalid CRC-7" << std::endl;
      return false;
   }

   psvcd::SendCMD_Hold(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x08);

   return true;
}

bool psvcd::CMD6_ERASE_GROUP_DEF_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD6_1" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_Poll(ftHandle, 0x46, 0x03, 0xAF, 0x01, 0x00, 0x43, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      if(resp.front() != 0x06)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD6_1 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD8_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD8_2" << std::endl;

   std::vector<BYTE> respRaw;
   psvcd::SendCMD_ReadResp_ReadData(ftHandle, 0x48, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x04, respRaw, 511 + 2); // (size is 512 + 2 bytes of CRC16 - 1 because FTDI counts 0 as one byte)

   if(respRaw.size())
   {
      std::vector<BYTE> cmdResp(respRaw.begin(), respRaw.begin() + 6);

      psvcd::PrintResp(cmdResp);

      if(cmdResp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(cmdResp);

      //validate

      if(respRaw.front() != 0x08)
         return false;

      uint8_t crc7 = psvcd::CRC_7(respRaw.data(), 5);
      bool valid = crc7 == respRaw[5];

      std::cout << "valid: " << valid << std::endl;

      if(!valid)
         return false;

      if(respRaw.size() < 520)
         return false;

      std::vector<BYTE> data(respRaw.begin() + 6, respRaw.end() - 2);

      if(data.size() < 512)
         return false;
      
      uint16_t crc16Expected = psvcd::CRC_16(data.data(),data.size());
      uint16_t crc16Read = (respRaw[518] << 8) | respRaw[519];

      if(crc16Expected != crc16Read)
      {
         std::cout << "CRC is not valid. expected: " << crc16Expected << " read: " << crc16Read << std::endl;
         return false;
      }
      else
      {
         std::cout << "CRC is correct: " << crc16Read << std::endl;
      }

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD8 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD13_MMC(FT_HANDLE ftHandle, WORD RCA, psvcd::MMC_CardStatusInfo& info)
{
   std::cout << "CMD13" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x4D, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp);

   if(resp.size())
   {
      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      psvcd::PrintResp(resp);

      GetMMCStatus(resp, info);

      if(resp.front() != 0x0D)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      return valid;
   }
   else
   {
      std::cout << "CMD13 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD6_BUS_WIDTH_MMC(FT_HANDLE ftHandle, BYTE width)
{
   std::cout << "CMD6_3" << std::endl;

   //34-If the power class, for the chosen width, is different from the default power class, send CMD6, and
   //write the POWER_CLASS byte of the EXT_CSD with the required power class.
   //35-The card might signal BUSY after CMD6; wait for the card to be out of BUSY
   //36-Send CMD6, writing the BUS_WIDTH byte of the EXT_CSD with the chosen bus width. An argument
   //of 0x03B7_0100 will set a 4-bits bus, an argument 0x03B7_0200 will set an 8-bit bus.
   //37-The bus is ready to exchange data using the new width configuration.

   //0 will be 1-bit
   //1 will be 4-bit
   //5 will be 4-bit for dual data rate

   BYTE cmd[] = {0x46, 0x03, 0xB7, width, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_Poll(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      //validate

      if(resp.front() != 0x06)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD6_3 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD16_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD16" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, 0x50, 0x00, 0x00, 0x02, 0x00, 0x15, 0x04, resp);

   if(resp.size())
   {
      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      psvcd::PrintResp(resp);

      PrintMMCStatus(resp);

      if(resp.front() != 0x10)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD16 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD17_MMC(FT_HANDLE ftHandle, BYTE A3, BYTE A2, BYTE A1, BYTE A0, std::vector<BYTE>& resp_data)
{
   std::cout << "CMD17" << std::endl;

   BYTE cmd[] = {0x51, A3, A2, A1, A0, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_ReadData(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp, 511 + 2);

   if(resp.size())
   {
      //psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      if(resp.front() != 0x11)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool crc7_valid = crc7 == resp[5];

      if(crc7_valid)
      {
         std::cout << "responce crc is valid" << std::endl;
      }
      else
      {
         std::cout << "responce crc is invalid" << std::endl;
         return false;
      }

      if(resp.size() < 520)
         return false;

      std::vector<BYTE> data(resp.begin() + 6, resp.end() - 2);

      if(data.size() < 512)
         return false;

      uint16_t crc16_resp_exp = psvcd::CRC_16(data.data(), data.size());
      uint16_t crc16_resp = (resp[518] << 8) | resp[519];

      if(crc16_resp_exp == crc16_resp)
      {
         std::cout << "responce data crc is valid" << std::endl;
      }
      else
      {
         std::cout << "responce data crc is invalid" << std::endl;
         return false;
      }

      resp_data.assign(data.begin(), data.end());

      //TODO: need to check card state with CMD13 and calculate is_valid

      return true;
   }
   else
   {
      std::cout << "CMD17 no response" << std::endl;
      return false;
   }
}

//TODO: In some cases CMD12 may require DAT0 busy polling - need to figure out this since sometimes CMD12 fails
bool psvcd::CMD12_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD12" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x61, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      if(resp.front() != 0x0C)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD12 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD18_MMC(FT_HANDLE ftHandle, BYTE A3, BYTE A2, BYTE A1, BYTE A0, uint32_t BperS, int32_t SperC, std::vector<BYTE>& resp_data) //resp size must be equal to nSectors * bytePerSector
{
   std::cout << "CMD18" << std::endl;

   BYTE cmd[] = {0x52, A3, A2, A1, A0, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_ReadDataMultiple(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp, (BperS - 1 + 2), SperC); // -1 because of weird ftdi length that starts from 0 / +2 is for crc16

   if(resp.size())
   {
      //PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      if(resp.front() != 0x12)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool crc7_valid = crc7 == resp[5];
      
      if(crc7_valid)
      {
         std::cout << "responce crc is valid" << std::endl;
      }
      else
      {
         std::cout << "responce crc is invalid" << std::endl;
         return false;
      }

      if(resp.size() < (((BperS + 2) * SperC) + 6))
         return false;

      resp_data.reserve(BperS * SperC);

      std::vector<BYTE>::iterator datait = resp.begin() + 6;
      for(int32_t  i = 0; i < SperC; i++)
      {
         uint16_t crc16_resp_exp = psvcd::CRC_16(&(*datait), BperS);
         uint16_t crc16_resp = ((*(datait + BperS)) << 8) | (*(datait + BperS + 1));

         if(crc16_resp_exp != crc16_resp)
         {
            std::cout << "responce data crc is invalid" << std::endl;
            return false;
         }
         else
         {
            resp_data.insert(resp_data.end(), datait, datait + BperS);
         }

         datait = datait + BperS + 2;
      }

      std::cout << "responce data crc is valid" << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return true;
   }
   else
   {
      std::cout << "CMD18 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD23_MMC(FT_HANDLE ftHandle, BYTE S3, BYTE S2, BYTE S1, BYTE S0)
{
   std::cout << "CMD23" << std::endl;

   BYTE cmd[] = {0x57, S3, S2, S1, S0, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      if(resp.front() != 0x17)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool crc7_valid = crc7 == resp.back();

      //TODO: need to check card state with CMD13 and calculate is_valid

      if(crc7_valid)
      {
         std::cout << "responce crc is valid" << std::endl;
         return true;
      }
      else
      {
         std::cout << "responce crc is invalid" << std::endl;
         return false;
      }
   }
   else
   {
      std::cout << "CMD23 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD6_Enable_HS_TIMING_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD6_2" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_Poll(ftHandle, 0x46, 0x03, 0xB9, 0x01, 0x00, 0x2F, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      //validate

      if(resp.front() != 0x06)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD6_3 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD6_Disable_HS_TIMING_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD6_2" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_Poll(ftHandle, 0x46, 0x03, 0xB9, 0x00, 0x00, 0x39, 0x04, resp);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(resp);

      //validate

      if(resp.front() != 0x06)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool valid = crc7 == resp.back();

      std::cout << "valid: " << valid << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return valid;
   }
   else
   {
      std::cout << "CMD6_3 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD15_MMC(FT_HANDLE ftHandle, WORD RCA)
{
   std::cout << "CMD15" << std::endl;

   BYTE RCAlo = (BYTE)(RCA & 0xFF);
   BYTE RCAhi = (BYTE)(RCA >> 8);
   BYTE cmd[] = {0x4F, RCAhi, RCAlo, 0x00, 0x00, 0x00};
   uint8_t crc7 = psvcd::CRC_7(cmd, 5);
   cmd[5] = crc7;
      
   psvcd::SendCMD_Hold(ftHandle, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], 0x08);
   return true;
}

//TODO: not sure if this command is supported by PS Vita game carts
bool psvcd::CMD19_MMC(FT_HANDLE ftHandle)
{
   std::cout << "CMD19" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp(ftHandle, 0x53, 0x00, 0x00, 0x00, 0x00, 0x8d, 0x04, resp);

   if(resp.size())
   {
      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      psvcd::PrintResp(resp);

      PrintMMCStatus(resp);

      //TODO: need to check crc7 of responce
      //TODO: need to check card state with CMD13 and calculate is_valid

      return true;
   }
   else
   {
      std::cout << "CMD19 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD56_MMC_1_WRITE(FT_HANDLE ftHandle, const std::vector<uint8_t>& packet)
{
   std::cout << "CMD56" << std::endl;

   std::vector<BYTE> resp;
   psvcd::SendCMD_ReadResp_SendData(ftHandle, 0x78, 0x00, 0x00, 0x00, 0x00, 0x25, 0x04, resp, packet);

   if(resp.size())
   {
      psvcd::PrintResp(resp);

      //validate
      if(resp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      if(resp.front() != 0x38)
         return false;

      uint8_t crc7 = psvcd::CRC_7(resp.data(), 5);
      bool crc7_valid = crc7 == resp[5];

      std::cout << "valid: " << crc7_valid << std::endl;

      if(!crc7_valid)
         return false;

      std::vector<BYTE> stuff(resp.begin() + 6, resp.end());
      std::cout << "stuff: " << std::hex << (int)stuff[0] << " " << (int)stuff[1] << " " << (int)stuff[2] << " " << (int)stuff[3] << " " << std::endl;

      //TODO: need to check card state with CMD13 and calculate is_valid

      return true;
   }
   else
   {
      std::cout << "CMD56 no response" << std::endl;
      return false;
   }
}

bool psvcd::CMD56_MMC_1_READ(FT_HANDLE ftHandle, std::vector<BYTE>& resp_data)
{
   std::cout << "CMD56" << std::endl;

   std::vector<BYTE> respRaw;
   psvcd::SendCMD_ReadResp_ReadData(ftHandle, 0x78, 0x00, 0x00, 0x00, 0x01, 0x37, 0x04, respRaw, 511 + 2); // (size is 512 + 2 bytes of CRC16)

   if(respRaw.size())
   {
      std::vector<BYTE> cmdResp(respRaw.begin(), respRaw.begin() + 6);

      psvcd::PrintResp(cmdResp);

      if(cmdResp.size() < 6)
      {
         std::cout << "Wrong response size" << std::endl;
         return false;
      }

      PrintMMCStatus(cmdResp);

      //validate

      if(cmdResp.front() != 0x38)
         return false;

      uint8_t crc7 = psvcd::CRC_7(cmdResp.data(), 5);
      bool crc7_valid = crc7 == cmdResp[5];

      std::cout << "valid: " << crc7_valid << std::endl;

      if(!crc7_valid)
         return false;

      if(respRaw.size() < 520)
         return false;

      std::vector<BYTE> data(respRaw.begin() + 6, respRaw.end() - 2);

      if(data.size() < 512)
         return false;
      
      uint16_t crc16Expected = psvcd::CRC_16(data.data(),data.size());
      uint16_t crc16Read = (respRaw[518] << 8) | respRaw[519];

      if(crc16Expected != crc16Read)
      {
         std::cout << "CRC is not valid. expected: " << crc16Expected << " read: " << crc16Read << std::endl;
         return false;
      }
      else
      {
         std::cout << "CRC is correct: " << crc16Read << std::endl;
      }

      resp_data.assign(data.begin(), data.end());

      //TODO: need to check card state with CMD13 and calculate is_valid

      return true;
   }
   else
   {
      std::cout << "CMD56 no response" << std::endl;
      return false;
   }
}