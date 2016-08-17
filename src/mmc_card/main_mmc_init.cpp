#include <iostream>
#include <string>
#include <sstream>
#include <stdint.h>
#include <fstream>

#include <chrono>
#include <thread>
#include <vector>

#include <ftd2xx.h>

#include <common/FTDICommon.h>
#include <common/FTDIInitialization.h>

#include "MMCInitialization.h"

#define FTDI_DEVICE_STRING_ID "USB FIFO"

bool OpenDevice2(FT_HANDLE& ftHandle)
{
   int index = psvcd::GetDeviceIndex(FTDI_DEVICE_STRING_ID);

   if(index < 0)
      return false; 

   if(!psvcd::OpenDevice(index, ftHandle))
      return false;

   if(psvcd::ConfigureFTDIPort(ftHandle) < 0)
      return false;

    if(psvcd::SyncMMPSE(ftHandle) < 0)
       return false;

   if(!psvcd::ConfigureSettings(ftHandle))
      return false; 

   return true;
}

int main_mmc_init(int argc, char* argv[])
{
   FT_HANDLE ftHandle;

   if(!OpenDevice2(ftHandle))
      return -1;

   //try to initialize mmc card
   psvcd::InitializeMMCCard(ftHandle);

   if(!psvcd::CloseDevice(ftHandle))
      return -1;
   
	return 0;
}