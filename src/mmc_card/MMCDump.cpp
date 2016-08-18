#include <windows.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>

#include <ftd2xx.h>

#include <common/CardRoutines.h>
#include <common/FTDIInitialization.h>

#include <dump_exfat/ExFatTypes.h>

#include "MMCDump.h"
#include "MMCCommands.h"

#define DEFAULT_PS_VITA_RCA 0x0001

bool RetryDeselectSelectCard(FT_HANDLE ftHandle)
{
   //deselect the card - at this point there will be no responce from the card so we do not need to check it
   bool res = psvcd::CMD7_MMC_Deselect(ftHandle);

   //retry selecting card
   int i = 0;
   for(i; i < 2; i++)
   {
      //enter transfer state
      //we are lucky enough here because re-selecting the card does not require CMD56 init sequence again
      if(psvcd::CMD7_MMC(ftHandle, DEFAULT_PS_VITA_RCA))
         break;

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::cout << "CMD7 error" << std::endl;
   }
   
   if(i == 2)
   {
      std::cout << "Failed to select the card" << std::endl;
      return false;
   }

   return true;
}

//==========================================================

bool RetryCMD12(FT_HANDLE ftHandle)
{
   int i = 0;
   for(i; i < 2; i++)
   {
      if(psvcd::CMD12_MMC(ftHandle))
         break;

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::cout << "CMD12 error" << std::endl;
   }

   if(i == 2)
   {
      std::cout << "Failed to terminate transmission" << std::endl;
      return false;
   }

   return true;
}

bool RetryCMD13(FT_HANDLE ftHandle, psvcd::MMC_CardStatusInfo& cardInfo)
{
   int i = 0;
   for(i; i < 2; i++)
   {
      if(CMD13_MMC(ftHandle, DEFAULT_PS_VITA_RCA, cardInfo)) //RCA that used is always 1
         break;

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::cout << "CMD13 error" << std::endl;
   }

   if(i == 2)
   {
      std::cout << "Failed to get the card state" << std::endl;
      return false;
   }

   return true;
}

//==========================================================

//there can be 2 states
//either card is in data state because previous tranmission has failed
//or COM_CRC_ERROR happened
//in first case we need to send CMD12 - STOP_TRANSMISSION
//in second case card will forever stay in trans state
//so we need to be a bit more tricky here
//send CMD7 with 0x0000 to deselect the card and go in standby mode
//then select card again with CMD7 0x0001 default RCA

bool HandleErrorState(FT_HANDLE ftHandle)
{
   //get card state
   psvcd::MMC_CardStatusInfo cardInfo;

   if(!RetryCMD13(ftHandle, cardInfo)) 
   {
      std::cout << "Failed to get card state - deselecting/selecting card" << std::endl;

      if(!RetryDeselectSelectCard(ftHandle))
         return false; // the only choice left is to restart ft232h

      //if we successfuly deselected/selected card - we may try to get state of the card again - but only once!
   
      //get card state again
      if(!RetryCMD13(ftHandle, cardInfo))
         return false; // the only choice left is to restart ft232h
   }

   //check if card is in data state - that can happen when CMD23 has failed and open ended transmission had started
   if(cardInfo.currentState == psvcd::data)
   {
      std::cout << "Card is in data state - terminating transmission" << std::endl;

      if(!RetryCMD12(ftHandle))
         return false; // the only choice left is to restart ft232h
   }

   //if card was in data state - successfuly terminated transmission

   //there can be also other errors when sending commands
   //in this case the only solution seem to be deselecting and selecting card again
   //since in such case card does not respond to other commands   
   if(cardInfo.comCrcError)
   {
      std::cout << "Card is in COM_CRC_ERROR state - deselecting/selecting card" << std::endl;

      if(!RetryDeselectSelectCard(ftHandle))
         return false; // the only choice left is to restart ft232h
   }
   else
   {
      //sometimes it happens that CMD12 fails to terminate transmission
      //but somehow card state is not data anymore
      //but instead we get illegal command
      if(cardInfo.illegalCommand)
      {
         std::cout << "Card is in ILLEGAL_COMMAND state - deselecting/selecting card" << std::endl;

         if(!RetryDeselectSelectCard(ftHandle))
            return false; // the only choice left is to restart ft232h
      }
      else
      {
         //TODO: just a general check for now to catch other errors - not yet sure how to handle them
         if(cardInfo.is_invalid())
         {
            std::cout << "Other unhandled error" << std::endl;
            return false;
         }
      }
   }

   return true;
}

//==========================================================

bool RetryCMD17(FT_HANDLE ftHandle, uint32_t address, std::vector<BYTE>& resp_data)
{
   BYTE a3 = (address & 0xFF000000) >> 24;
   BYTE a2 = (address & 0x00FF0000) >> 16;
   BYTE a1 = (address & 0x0000FF00) >> 8;
   BYTE a0 = (address & 0x000000FF) >> 0;

   int i = 0;
   for(i; i < 2; i++)
   {
      if(psvcd::CMD17_MMC(ftHandle, a3, a2, a1, a0, resp_data))
         break;

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      std::cout << "CMD17 error" << std::endl;
      resp_data.clear();
   }

   if(i == 2)
   {
      std::cout << "Failed to dump cluster " << address << std::endl;
      return false;
   }

   return true;
}

bool CMD23_18_Retry(FT_HANDLE ftHandle, int32_t SperC, int32_t BperS, uint32_t cluster, std::vector<BYTE>& resp_data)
{
   //std::this_thread::sleep_for(std::chrono::milliseconds(50));

   uint32_t address = cluster * SperC;

   BYTE a3 = (address & 0xFF000000) >> 24;
   BYTE a2 = (address & 0x00FF0000) >> 16;
   BYTE a1 = (address & 0x0000FF00) >> 8;
   BYTE a0 = (address & 0x000000FF) >> 0;

   BYTE s3 = (SperC & 0xFF000000) >> 24;
   BYTE s2 = (SperC & 0x00FF0000) >> 16;
   BYTE s1 = (SperC & 0x0000FF00) >> 8;
   BYTE s0 = (SperC & 0x000000FF) >> 0;

   bool valid = false;

   int errctr = 0;

   while(!valid)
   {
      if(errctr == 2)
      {
         std::cout << "Completely failed to dump sector " << address << std::endl;
         return false; //the only way here would be to restart ft232h
      }

      valid = psvcd::CMD23_MMC(ftHandle, s3, s2, s1, s0);

      if(!valid)
      {
         std::cout << "CMD23 error" << std::endl;
         
         //wait some time so that card switches to "open-ended multiple block read"
         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
         
         //I have figured out that at this point trying different ways to recover the transmission will not help
         //it would be just a serious waste of time - the only solution here is to restart ft232h
         return false;

         //try to recover in different ways
         if(!HandleErrorState(ftHandle))
            return false; //the only way here would be to restart ft232h
         
         //try to read again
         errctr++;
         continue;
      }
      
      valid = psvcd::CMD18_MMC(ftHandle, a3, a2, a1, a0, BperS, SperC, resp_data);

      if(!valid)
      {
         std::cout << "CMD18 error" << std::endl;
         
         resp_data.clear();

         //wait some time so that card switches to "open-ended multiple block read"
         std::this_thread::sleep_for(std::chrono::milliseconds(1000));

         //I have figured out that at this point trying different ways to recover the transmission will not help
         //it would be just a serious waste of time - the only solution here is to restart ft232h
         return false;

         //try to recover in different ways
         if(!HandleErrorState(ftHandle))
            return false; //the only way here would be to restart ft232h

         //try to read again
         errctr++;
         continue;
      }
   }

   return true;
}

//==========================================================

bool DumpFsSonyRoot(FT_HANDLE ftHandle, psvcd::FsSonyRoot* root)
{
   std::vector<BYTE> resp_data;
   if(!RetryCMD17(ftHandle, 0x00000000, resp_data))
      return false;

   memset(root, 0, sizeof(psvcd::FsSonyRoot));
   memcpy(root, resp_data.data(), resp_data.size());

   return true;
}

bool DumpVBR(FT_HANDLE ftHandle, psvcd::VBR* vbr, uint32_t vbr_address)
{
   std::vector<BYTE> resp_data;
   if(!RetryCMD17(ftHandle, vbr_address, resp_data))
      return false;

   memset(vbr, 0, sizeof(psvcd::VBR));
   memcpy(vbr, resp_data.data(), resp_data.size());

   return true;
}

//==========================================================

bool DumpByCluster(FT_HANDLE ftHandle, const psvcd::FsSonyRoot& fsRoot, const psvcd::VBR& vbr, uint32_t initialCluster, uint32_t& failedCluster, std::string filePath)
{
   int32_t BperS = (int32_t)pow(2, vbr.BytesPerSectorShift);
   int32_t SperC = (int32_t)pow(2, vbr.SectorsPerClusterShift);

   uint32_t clusters = (uint32_t)(vbr.VolumeLength / SperC); // card supports only 32 bit cluster address
   uint32_t clustersTail = vbr.VolumeLength % SperC; //should be zero technically

   std::ofstream str(filePath,  std::ios::out | std::ios::app | std::ios::binary);

   for(uint32_t i = initialCluster; i < clusters; i++)
   {
      std::vector<BYTE> resp_data;

      bool res = CMD23_18_Retry(ftHandle, SperC, BperS, i, resp_data);

      if(!res)
      {
         failedCluster = i;
         return false;
      }

      BYTE* raw_data = resp_data.data();
      str.write((const char*)raw_data, resp_data.size());

      std::cout << i << " from " << clusters << std::endl;
   }

   return true;
}

//when card is first hardwired to custom board we have to exit from high speed mode and change bus width from 4 to one
//since custom board does not support too high frequences and 4 bit bus width
bool EnterDumpableModeInternal(FT_HANDLE ftHandle)
{
   //first - switch to low speed mode
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //just ignore errors here - there will be no valid responce
   //because responce by some reason is not alighed to the clock rising edge
   psvcd::CMD6_Disable_HS_TIMING_MMC(ftHandle);

   //then switch to 1 bit bus
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //set BUS_WIDTH to 1 in EXT_CSD
   if(!psvcd::CMD6_BUS_WIDTH_MMC(ftHandle, 0))
      return false;

   return true;
}

bool psvcd::EnterDumpableMode(FT_HANDLE ftHandle)
{
   WORD dwClockDivisor = 0x0034;

   //pull all lines to high except clock
   if(!psvcd::SetLinesIdle(ftHandle))
      return false;

   //set valid frequency
   if(!psvcd::ConfigureDivisor(ftHandle, dwClockDivisor))
      return false;

   if(!EnterDumpableModeInternal(ftHandle))
      return false;

   return true;
}

bool psvcd::RawBlockDumpMMCCard(FT_HANDLE ftHandle, uint32_t initialCluster, uint32_t& failedCluster, std::string filePath, bool& lowFreqRequired)
{
   //this is the last devisor that can be handled by the custom board and low speed MMC transfer
   //any frequency that is faster will cause errors duing transmission

   //WORD dwClockDivisor = 0x0010;  //1.76 MHz

   //table of higher frequences:

   //WORD dwClockDivisor = 0x000A;  //2.72 MHz
   //WORD dwClockDivisor = 0x0009;  //3 MHz
   //WORD dwClockDivisor = 0x0008;  //3.33 MHz
   //WORD dwClockDivisor = 0x0007;  //3.37 MHz
   //WORD dwClockDivisor = 0x0006;  //4.28 MHz
   //WORD dwClockDivisor = 0x0005;  //5.00 MHz
   
   //WORD dwClockDivisor = 0x0004;  //6.00 MHz
   //WORD dwClockDivisor = 0x0003;  //7.50 MHz
   //WORD dwClockDivisor = 0x0002;  //10.00 MHz
   //WORD dwClockDivisor = 0x0001;  //15.00 MHz
   //WORD dwClockDivisor = 0x0000;  //30.00 MHz

   //TODO: I am not sure if switching frequences actually helps to avoid fatal errors
   //switch to low freq during initialization
   WORD dwClockDivisor = 0x0000;
   if(lowFreqRequired)
      dwClockDivisor = 0x0034;
   else 
      dwClockDivisor = 0x0010; //the lowest devisor at which custom board does not give lags and errors

   //pull all lines to high except clock
   if(!psvcd::SetLinesIdle(ftHandle))
   {
      failedCluster = initialCluster;
      return false;
   }

   //set frequency
   if(!psvcd::ConfigureDivisor(ftHandle, dwClockDivisor))
   {
      failedCluster = initialCluster;
      return false;
   }

   //handle any errors that may have occured during previous dump iteration
   //this may include deselecting/selecting card
   //terminating previous read operation with CMD12
   if(!HandleErrorState(ftHandle))
   {
      failedCluster = initialCluster;
      return false;
   }

   //switch back to high freq - if required
   if(lowFreqRequired)
   {
      dwClockDivisor = 0x0010;
      lowFreqRequired = false;
   }

   FsSonyRoot fsRoot;
   if(!DumpFsSonyRoot(ftHandle, &fsRoot))
   {
      failedCluster = initialCluster;
      return false;
   }

   //sometimes card just goes crazy without any indication of the error or the status
   //at least we can check some headers and try deselect/select the card
   //it looks like switching to low frequency for a moment duing re initialization actually helps (not sure)
   //TODO: I am not sure if switching frequency actually helps
   if(std::string((const char*)fsRoot.SCEIid, 32) != SCEIidConstant)
   {
      std::cout << "SCEI id header is invalid" << std::endl;

      failedCluster = initialCluster;

      lowFreqRequired = true;

      if(!RetryDeselectSelectCard(ftHandle))
         return false; // the only choice left is to restart ft232h

      //still need to restart because header was read incorrectly
      //and we will not be able to get correct VBR sector address
      return false;
   }

   VBR vbr;
   if(!DumpVBR(ftHandle, &vbr, fsRoot.FsOffset))
   {
      failedCluster = initialCluster;
      return false;
   }

   if(std::string((const char*)vbr.FileSystemName, 8) != EXFATContant)
   {
      std::cout << "EXFAT header is invalid" << std::endl;

      failedCluster = initialCluster;

      lowFreqRequired = true;

      if(!RetryDeselectSelectCard(ftHandle))
         return false; // the only choice left is to restart ft232h

      //still need to restart because header was read incorrectly
      //and we will not be able to get correct "byte per sector" and "sector per cluster" fields
      return false;
   }

   return DumpByCluster(ftHandle, fsRoot, vbr, initialCluster, failedCluster, filePath);
}