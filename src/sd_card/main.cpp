#include <ftd2xx.h>

#include <common/FTDICommon.h>
#include <common/FTDIInitialization.h>

#include "SDInitialization.h"

#define FTDI_DEVICE_STRING_ID "USB FIFO"

bool OpenDevice(FT_HANDLE& ftHandle)
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

int main(int argc, char* argv[])
{
   FT_HANDLE ftHandle;

   if(!OpenDevice(ftHandle))
      return -1;

   //try initialize sd card
   psvcd::InitializeSDCard(ftHandle);

   if(!psvcd::CloseDevice(ftHandle))
      return -1;
   
	return 0;
}