#pragma once

#include <windows.h>

#include <ftd2xx.h>

namespace psvcd {

int ConfigureFTDIPort(FT_HANDLE ftHandle);

int SyncMMPSE(FT_HANDLE ftHandle);

bool ConfigureSettings(FT_HANDLE ftHandle);

bool WaitCycles(FT_HANDLE ftHandle, WORD nCycles);

bool WaitPowerUp(FT_HANDLE ftHandle);

bool ConfigureDivisor(FT_HANDLE ftHandle, WORD dwClockDivisor);

};