#pragma once

#include <windows.h>
#include <stdint.h>
#include <vector>

#include <ftd2xx.h>

#include "MMCTypes.h"

namespace psvcd {

bool CMD0_MMC(FT_HANDLE ftHandle);

bool CMD1_MMC(FT_HANDLE ftHandle);

bool CMD2_MMC(FT_HANDLE ftHandle);

bool CMD3_MMC(FT_HANDLE ftHandle, WORD RCA);

bool CMD9_MMC(FT_HANDLE ftHandle, WORD RCA);

bool CMD7_MMC(FT_HANDLE ftHandle, WORD RCA);

bool CMD7_MMC_Deselect(FT_HANDLE ftHandle);

bool CMD6_ERASE_GROUP_DEF_MMC(FT_HANDLE ftHandle);

bool CMD8_MMC(FT_HANDLE ftHandle);

bool CMD13_MMC(FT_HANDLE ftHandle, WORD RCA, MMC_CardStatusInfo& info);

bool CMD6_BUS_WIDTH_MMC(FT_HANDLE ftHandle, BYTE width);

bool CMD16_MMC(FT_HANDLE ftHandle);

bool CMD17_MMC(FT_HANDLE ftHandle, BYTE A3, BYTE A2, BYTE A1, BYTE A0, std::vector<BYTE>& resp_data);

bool CMD12_MMC(FT_HANDLE ftHandle);

bool CMD18_MMC(FT_HANDLE ftHandle, BYTE A3, BYTE A2, BYTE A1, BYTE A0, uint32_t BperS, int32_t SperC, std::vector<BYTE>& resp_data);
bool CMD23_MMC(FT_HANDLE ftHandle, BYTE S3, BYTE S2, BYTE S1, BYTE S0);

bool CMD6_Enable_HS_TIMING_MMC(FT_HANDLE ftHandle);

bool CMD6_Disable_HS_TIMING_MMC(FT_HANDLE ftHandle);

bool CMD15_MMC(FT_HANDLE ftHandle, WORD RCA);

bool CMD19_MMC(FT_HANDLE ftHandle);

bool CMD56_MMC_1_WRITE(FT_HANDLE ftHandle, const std::vector<uint8_t>& packet);

bool CMD56_MMC_1_READ(FT_HANDLE ftHandle, std::vector<BYTE>& resp_data);

};