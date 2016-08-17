#pragma once

#include <windows.h>
#include <ftd2xx.h>
#include <vector>

namespace psvcd {

bool CMD0_SD(FT_HANDLE ftHandle);

bool CMD8_SD(FT_HANDLE ftHandle);

bool CMD55_SD(FT_HANDLE ftHandle);

bool ACMD41_SD(FT_HANDLE ftHandle, std::vector<BYTE>& resp);

bool CMD55_ACMD41_SD(FT_HANDLE ftHandle);

bool CMD2_SD(FT_HANDLE ftHandle);

bool CMD3_SD(FT_HANDLE ftHandle, WORD& RCA);

bool CMD10_SD(FT_HANDLE ftHandle, WORD RCA);

bool CMD13_SD(FT_HANDLE ftHandle, WORD RCA);

bool CMD7_SD(FT_HANDLE ftHandle, WORD RCA);

bool CMD7_SD(FT_HANDLE ftHandle);

bool CMD17_SD(FT_HANDLE ftHandle, BYTE A3, BYTE A2, BYTE A1, BYTE A0);

};