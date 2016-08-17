#include <windows.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>

#include <ftd2xx.h>

#include <common/CardRoutines.h>
#include <common/FTDIInitialization.h>

#include "MMCInitialization.h"
#include "CMD56Initialization.h"
#include "MMCCommands.h"

//It is very interesting that PS Vita game cart can actually use any RCA
//This means that we can stack multiple carts on single data bus
//Which in theory may allow to bypass initialization with original cart and then use dump from any external device

//By default PS Vita uses 0x0001 but I tested couple of different values
#define PS_VITA_CUSTOM_RCA 0x0002

void SwitchToHighSpeedState(FT_HANDLE ftHandle, WORD RCA)
{
   //it is explained in spec that HS_TIMING must be set before BUS_WIDTH

   //http://www.spinics.net/lists/linux-mmc/msg33243.html
   //https://e2e.ti.com/support/dsp/omap_applications_processors/f/42/t/29607
   //http://www.samsung.com/global/business/semiconductor/file/product/mmc_high_speed_mode_setting_sequence_v1.0-0.pdf

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //set HS_TIMING in EXT_CSD
   if(!psvcd::CMD6_Enable_HS_TIMING_MMC(ftHandle))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return;
   }   
   
   //I see that after HS_TIMING response of the card changes - it does not match clock any longer
   //to be more precise - data becomes valid on falling edge instead of standard rising edge

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!psvcd::ConfigureDivisor(ftHandle, 0x0000)) // switch to highest possible speed
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));
   
   //set BUS_WIDTH to 4 in EXT_CSD
   if(!psvcd::CMD6_BUS_WIDTH_MMC(ftHandle, 1))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return;
   }
}

void psvcd::InitializeMMCCard(FT_HANDLE ftHandle)
{
   WORD dwClockDivisor1 = 0x0069; // initial step may require lower frequency (check documentation)
   WORD dwClockDivisor2 = 0x0034; // after initialization is done - we can switch to high frequency

   //pull all lines to high except clock
   if(!psvcd::SetLinesIdle(ftHandle))
      return;
   
   //set frequency
   if(!psvcd::ConfigureDivisor(ftHandle, dwClockDivisor1))
      return;

   //wait till card finishes power up
   if(!psvcd::WaitPowerUp(ftHandle))
      return;

   //send cmd0 again - card should wake up
   if(!CMD0_MMC(ftHandle))
      return;

   // TODO: I do not think that waiting is requred but I did not test tight timing because custom board does not support high frequences
   // Can do it later when high performance will be required
   std::this_thread::sleep_for(std::chrono::milliseconds(40)); 

   //wait till card initializes
   if(!CMD1_MMC(ftHandle))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //TODO: request CID - this should be parsed into corresponding structure to print some more info
   if(!CMD2_MMC(ftHandle))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //Assign RCA
   if(!CMD3_MMC(ftHandle, PS_VITA_CUSTOM_RCA))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //TODO: request CSD - this should be parsed into corresponding structure to print some more info
   if(!CMD9_MMC(ftHandle, PS_VITA_CUSTOM_RCA))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //enter transfer state
   if(!CMD7_MMC(ftHandle, PS_VITA_CUSTOM_RCA))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   MMC_CardStatusInfo cardInfo1;
   if(!CMD13_MMC(ftHandle, PS_VITA_CUSTOM_RCA, cardInfo1))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));
      
   //set ERASE_GROUP_DEF in EXT_CSD
   if(!CMD6_ERASE_GROUP_DEF_MMC(ftHandle))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //get contents of EXT_CSD - this should be parsed into corresponding structure to print some more info
   if(!CMD8_MMC(ftHandle))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //get card status
   MMC_CardStatusInfo cardInfo2;
   if(!CMD13_MMC(ftHandle, PS_VITA_CUSTOM_RCA, cardInfo2))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //set BUS_WIDTH to 1 in EXT_CSD
   if(!CMD6_BUS_WIDTH_MMC(ftHandle, 0))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      CMD15_MMC(ftHandle, PS_VITA_CUSTOM_RCA);
      return;
   }

   //TODO: this method is not yet shared due to content of CMD56 packets
   //CMD56Initialization(ftHandle, PS_VITA_CUSTOM_RCA);

   //TODO: this method does not work as expected yet
   //SwitchToHighSpeedState(ftHandle, PS_VITA_CUSTOM_RCA);

   return;
}
