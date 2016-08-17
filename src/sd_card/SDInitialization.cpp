#include <windows.h>
#include <chrono>
#include <thread>

#include "SDInitialization.h"
#include "SDCommands.h"

#include <common/CardRoutines.h>
#include <common/FTDIInitialization.h>

void psvcd::InitializeSDCard(FT_HANDLE ftHandle)
{
   WORD dwClockDivisor1 = 0x0069; // initial step may require lower frequency (check documentation)
   WORD dwClockDivisor2 = 0x0034; // after initialization is done - we can switch to high frequency

   if(!psvcd::SetLinesIdle(ftHandle))
      return;
   
   //set second frequency (faster)
   if(!psvcd::ConfigureDivisor(ftHandle, dwClockDivisor1))
      return;

   //wait till card finishes power up
   if(!psvcd::WaitPowerUp(ftHandle))
      return;

   //send cmd0 again - card should wake up
   if(!CMD0_SD(ftHandle))
      return;

   //I have very old 16 Mb SD card that does not support CMD8

   /*
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //request CID
   if(!CMD8_SD(ftHandle))
      return;
   */

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!CMD55_ACMD41_SD(ftHandle))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //request CID
   if(!CMD2_SD(ftHandle))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //may need to send command couple of times, check here
   //http://forum.osdev.org/viewtopic.php?f=1&t=29686&start=0

   //Assign RCA
   WORD RCA = 0;

   if(!CMD3_SD(ftHandle, RCA))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!CMD3_SD(ftHandle, RCA))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!CMD3_SD(ftHandle, RCA))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   //test RCA by requesting CID again
   if(!CMD10_SD(ftHandle, RCA))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!CMD13_SD(ftHandle, RCA))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!CMD7_SD(ftHandle, RCA))
      return;

   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   if(!CMD17_SD(ftHandle, 0x00, 0x00, 0x00, 0x00))
      return;

   return;
}