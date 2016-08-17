#pragma once

#include <windows.h>
#include <string>

#include <ftd2xx.h>

namespace psvcd {

struct DriverVersion
{
   union
   {
      struct
      {
         BYTE Build;
         BYTE Minor;
         WORD Major;
      } parts;

      DWORD Version;
   };
};

struct LibraryVersion
{
   union
   {
      struct
      {
         BYTE Build;
         BYTE Minor;
         BYTE Major;
         BYTE Reserved;
      } parts;

      DWORD Version;
   };
};

std::string ErrorCodeToString(FT_STATUS status);

std::string InfoFlagsToString(ULONG flags);

std::string DeviceTypeToString(ULONG type);

std::string DeviceIdToString(ULONG id);

bool OpenDevice(int devIndex, FT_HANDLE& ftHandle);

bool CloseDevice(FT_HANDLE ftHandle);

std::string DriverVersionToString(DWORD version);

std::string LibraryVersionToString(DWORD version);

bool SetBitMode(FT_HANDLE ftHandle, UCHAR mask, UCHAR mode);

bool GetBitMode(FT_HANDLE ftHandle);

bool SetLatencyTimer(FT_HANDLE ftHandle, UCHAR ucLatency);

bool SetUSBParameters(FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize);

bool SetFlowControl(FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);

bool Purge(FT_HANDLE ftHandle, ULONG Mask);

bool ResetDevice(FT_HANDLE ftHandle);

bool CyclePort(FT_HANDLE ftHandle);

bool SetTimeouts(FT_HANDLE ftHandle, DWORD dwReadTimeout, DWORD dwWriteTimeout);

bool SetChars(FT_HANDLE ftHandle, UCHAR uEventCh, UCHAR uEventChEn, UCHAR uErrorCh, UCHAR uErrorChEn);

int GetDeviceIndex(std::string desc);

bool PrintDevices();

bool PrintDriverVersion(FT_HANDLE ftHandle);

bool PrintLibraryVersion();

bool PrintComPortNumber(FT_HANDLE ftHandle);

};