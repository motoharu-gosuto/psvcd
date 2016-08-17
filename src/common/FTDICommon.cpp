#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "FTDICommon.h"

std::string psvcd::ErrorCodeToString(FT_STATUS status)
{
   switch(status)
   {
   case FT_OK:
      return "OK";
   case FT_INVALID_HANDLE:
      return "INVALID_HANDLE";
   case FT_DEVICE_NOT_FOUND:
      return "DEVICE_NOT_FOUND";
   case FT_DEVICE_NOT_OPENED:
      return "DEVICE_NOT_OPENED";
   case FT_IO_ERROR:
      return "IO_ERROR";
   case FT_INSUFFICIENT_RESOURCES:
      return "INSUFFICIENT_RESOURCES";
   case FT_INVALID_PARAMETER:
      return "INVALID_PARAMETER";
   case FT_INVALID_BAUD_RATE:
      return "INVALID_BAUD_RATE";
   case FT_DEVICE_NOT_OPENED_FOR_ERASE:
      return "DEVICE_NOT_OPENED_FOR_ERASE";
   case FT_DEVICE_NOT_OPENED_FOR_WRITE:
      return "DEVICE_NOT_OPENED_FOR_WRITE";
   case FT_FAILED_TO_WRITE_DEVICE:
      return "FAILED_TO_WRITE_DEVICE";
   case FT_EEPROM_READ_FAILED:
      return "EEPROM_READ_FAILED";
   case FT_EEPROM_WRITE_FAILED:
      return "EEPROM_WRITE_FAILED";
   case FT_EEPROM_ERASE_FAILED:
      return "EEPROM_ERASE_FAILED";
   case FT_EEPROM_NOT_PRESENT:
      return "EEPROM_NOT_PRESENT";
   case FT_EEPROM_NOT_PROGRAMMED:
      return "EEPROM_NOT_PROGRAMMED";
   case FT_INVALID_ARGS:
      return "INVALID_ARGS";
   case FT_NOT_SUPPORTED:
      return "NOT_SUPPORTED";
   case FT_OTHER_ERROR:
      return "OTHER_ERROR";
   case FT_DEVICE_LIST_NOT_READY:
      return "DEVICE_LIST_NOT_READY";
   default:
      return "Unknown";
   }
}

std::string psvcd::InfoFlagsToString(ULONG flags)
{
   std::stringstream ss;
   if((flags & FT_FLAGS_OPENED) > 0)
      ss << "OPENED";

   if((flags & FT_FLAGS_HISPEED) > 0)
   {
      if(ss.str().length() > 0)
         ss << "| HISPEED";
      else
         ss << "HISPEED";
   }

   return ss.str();
}

std::string psvcd::DeviceTypeToString(ULONG type)
{
   switch(type)
   {
   case FT_DEVICE_BM:
      return "BM";
   case FT_DEVICE_AM:
      return "AM";
   case FT_DEVICE_100AX:
      return "100AX";
   case FT_DEVICE_UNKNOWN:
      return "UNKNOWN";
   case FT_DEVICE_2232C:
      return "2232C";
   case FT_DEVICE_232R:
      return "232R";
   case FT_DEVICE_2232H:
      return "2232H";
   case FT_DEVICE_4232H:
      return "4232H";
   case FT_DEVICE_232H:
      return "232H";
   case FT_DEVICE_X_SERIES:
      return "X_SERIES";
   case FT_DEVICE_4222H_0:
      return "4222H_0";
   case FT_DEVICE_4222H_1_2:
      return "4222H_1_2";
   case FT_DEVICE_4222H_3:
      return "4222H_3";
   case FT_DEVICE_4222_PROG:
      return "4222_PROG";
   case FT_DEVICE_900:
      return "900";
   default:
      return "Unknown";
   }
}

std::string psvcd::DeviceIdToString(ULONG id)
{
   std::stringstream ss;
   ss << "VID_" << std::hex << HIWORD(id) << "&" << "PID_" << std::hex << LOWORD(id);
   return ss.str();
}

bool psvcd::OpenDevice(int devIndex, FT_HANDLE& ftHandle)
{
   FT_STATUS ftStatus = FT_Open(devIndex, &ftHandle);

   if (ftStatus == FT_OK) 
   {
      std::cout << "Opened device" << std::endl;
      return true;
   }
   else 
   {
      std::cout << "Failed to open device" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::CloseDevice(FT_HANDLE ftHandle)
{
   FT_STATUS ftStatus = FT_Close(ftHandle);

   if (ftStatus == FT_OK) 
   {
      std::cout << "Closed device" << std::endl;
      return true;
   }
   else 
   {
      std::cout << "Failed to close device" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

std::string psvcd::DriverVersionToString(DWORD version)
{
   DriverVersion dwDriverVer;
   dwDriverVer.Version = version;

   std::stringstream ss;
   ss << std::hex << (DWORD)dwDriverVer.parts.Major << "." << std::hex << (DWORD)dwDriverVer.parts.Minor << "." << std::hex << (DWORD)dwDriverVer.parts.Build;
   return ss.str();
}

std::string psvcd::LibraryVersionToString(DWORD version)
{
   LibraryVersion dwLibraryVer;
   dwLibraryVer.Version = version;

   std::stringstream ss;
   ss<< std::hex << (DWORD)dwLibraryVer.parts.Major << "." << std::hex << (DWORD)dwLibraryVer.parts.Minor << "." << std::hex << (DWORD)dwLibraryVer.parts.Build;
   return ss.str();
}

bool psvcd::SetBitMode(FT_HANDLE ftHandle, UCHAR mask, UCHAR mode)
{
   FT_STATUS ftStatus;
      
   ftStatus = FT_SetBitMode(ftHandle, mask, mode); 

   if (ftStatus == FT_OK)
   {
      std::cout << "Set BitMode Complete" << std::endl;
      return true;
   } 
   else 
   {
      std::cout << "Failed to Set BitMode " << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::GetBitMode(FT_HANDLE ftHandle)
{
   UCHAR BitMode; 
   FT_STATUS ftStatus; 
   
   ftStatus = FT_GetBitMode(ftHandle, &BitMode); 

   if (ftStatus == FT_OK)
   {
      std::cout << "BitMode: 0x" << std::hex << (DWORD)BitMode << std::endl;
      return true;
   } 
   else 
   {
      std::cout << "Failed to Get BitMode " << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::SetLatencyTimer(FT_HANDLE ftHandle, UCHAR ucLatency)
{
   FT_STATUS ftStatus = FT_SetLatencyTimer(ftHandle, ucLatency);

   if (ftStatus == FT_OK) 
   {
      std::cout << "SetLatencyTimer complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to SetLatencyTimer" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::SetUSBParameters(FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize)
{
   FT_STATUS ftStatus = FT_SetUSBParameters(ftHandle, ulInTransferSize, ulOutTransferSize);

   if (ftStatus == FT_OK) 
   {
      std::cout << "SetUSBParameters complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to SetUSBParameters" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::SetFlowControl(FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar)
{
   FT_STATUS ftStatus = FT_SetFlowControl(ftHandle, FlowControl, XonChar, XoffChar);

   if (ftStatus == FT_OK) 
   {
      std::cout << "SetFlowControl complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to SetFlowControl" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::Purge(FT_HANDLE ftHandle, ULONG Mask)
{
   FT_STATUS ftStatus = FT_Purge(ftHandle, Mask);

   if (ftStatus == FT_OK) 
   {
      std::cout << "Purge complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to Purge" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::ResetDevice(FT_HANDLE ftHandle)
{
   FT_STATUS ftStatus = FT_ResetDevice(ftHandle);

   if (ftStatus == FT_OK) 
   {
      std::cout << "Reset complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to Reset" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::CyclePort(FT_HANDLE ftHandle)
{
   FT_STATUS ftStatus = FT_CyclePort(ftHandle);

   if (ftStatus == FT_OK) 
   {
      std::cout << "CyclePort complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to CyclePort" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::SetTimeouts(FT_HANDLE ftHandle, DWORD dwReadTimeout, DWORD dwWriteTimeout)
{
   FT_STATUS ftStatus = FT_SetTimeouts(ftHandle, dwReadTimeout, dwWriteTimeout);

   if (ftStatus == FT_OK) 
   {
      std::cout << "SetTimeouts complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to SetTimeouts" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::SetChars(FT_HANDLE ftHandle, UCHAR uEventCh, UCHAR uEventChEn, UCHAR uErrorCh, UCHAR uErrorChEn)
{
   FT_STATUS ftStatus = FT_SetChars(ftHandle, uEventCh, uEventChEn, uErrorCh, uErrorChEn);

   if (ftStatus == FT_OK) 
   {
      std::cout << "SetChars complete";
      return true;
   } 
   else 
   {
      std::cout << "Failed to SetChars" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

int psvcd::GetDeviceIndex(std::string desc)
{
   FT_STATUS ftStatus; 
   
   DWORD numDevs; // create the device information list 
   ftStatus = FT_CreateDeviceInfoList(&numDevs);

   if (ftStatus != FT_OK)
   {
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return -1;
   }

   if (numDevs <= 0)
      return -1;
   
   // allocate storage for list based on numDevs 
   FT_DEVICE_LIST_INFO_NODE* devInfo = new FT_DEVICE_LIST_INFO_NODE[numDevs];

   if(!devInfo)
   {
      delete [] devInfo;
      return -1;
   }
      
   // get the device information list 
   ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs); 

   if (ftStatus != FT_OK)
   {
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      delete [] devInfo;
      return -1;
   }
   
   for (DWORD i = 0; i < numDevs; i++) 
   { 
      if(devInfo[i].Description == desc)
      {
         delete [] devInfo;
         return i;
      }
   }

   delete [] devInfo;
   return -1;
}

bool psvcd::PrintDevices()
{
   FT_STATUS ftStatus; 
   
   DWORD numDevs; // create the device information list 
   ftStatus = FT_CreateDeviceInfoList(&numDevs);

   if (ftStatus != FT_OK)
   {
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
      
   std::cout << "Number of devices is " << numDevs << std::endl;

   if (numDevs <= 0)
      return false;
   
   // allocate storage for list based on numDevs 
   FT_DEVICE_LIST_INFO_NODE* devInfo = new FT_DEVICE_LIST_INFO_NODE[numDevs];

   if(!devInfo)
   {
      delete [] devInfo;
      return false;
   }
      
   // get the device information list 
   ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs); 

   if (ftStatus != FT_OK)
   {
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      delete [] devInfo;
      return false;
   }
   
   for (DWORD i = 0; i < numDevs; i++) 
   { 
      std::cout << "Dev:" << i << std::endl;
      std::cout << " Flags = 0x" << std::hex << devInfo[i].Flags << " (" << InfoFlagsToString(devInfo[i].Flags) << ")" << std::endl;
      std::cout << " Type = 0x" << std::hex << devInfo[i].Type << " (" << DeviceTypeToString(devInfo[i].Type) << ")" << std::endl;
      std::cout << " ID = 0x" << std::hex << devInfo[i].ID << " (" << DeviceIdToString(devInfo[i].ID) << ")" << std::endl;
      std::cout << " LocId = 0x" << std::hex << devInfo[i].LocId << std::endl;
      std::cout << " SerialNumber = " << devInfo[i].SerialNumber << std::endl;
      std::cout << " Description = " << devInfo[i].Description << std::endl;
      std::cout << " ftHandle = 0x" << std::hex << devInfo[i].ftHandle << std::endl;
   }

   delete [] devInfo;
   return true;
}

bool psvcd::PrintDriverVersion(FT_HANDLE ftHandle)
{
   FT_STATUS ftStatus;
   DWORD dwDriverVer;

   ftStatus = FT_GetDriverVersion(ftHandle, &dwDriverVer);
   if (ftStatus == FT_OK)
   {
      std::cout << "Driver version = 0x" << std::hex << dwDriverVer << " (" << DriverVersionToString(dwDriverVer) << ")" << std::endl;
      return true;
   }
   else 
   {
      std::cout << "Error reading driver version" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::PrintLibraryVersion()
{
   FT_STATUS ftStatus; 
   DWORD dwLibraryVer;
   
   // Get DLL version 
   ftStatus = FT_GetLibraryVersion(&dwLibraryVer);

   if (ftStatus == FT_OK)
   {
      std::cout << "Library version = 0x" << std::hex << dwLibraryVer << " (" << LibraryVersionToString(dwLibraryVer) << ")" << std::endl;
      return true;
   }
   else
   {
      std::cout << "Error reading library version" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}

bool psvcd::PrintComPortNumber(FT_HANDLE ftHandle)
{
   FT_STATUS ftStatus; 
   LONG lComPortNumber;

   ftStatus = FT_GetComPortNumber(ftHandle, &lComPortNumber); 

   if (ftStatus == FT_OK) 
   {
      if (lComPortNumber == -1) 
      {
         std::cout << "No COM port is assigned" << std::endl;
         return false;
      } 
      else 
      {
         std::cout << "Assigned COM port number is " << lComPortNumber << std::endl;
         return true;
      } 
   } 
   else
   {
      std::cout << "Failed to get COM port number" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
}