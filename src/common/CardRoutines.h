#pragma once

#include <windows.h>
#include <ftd2xx.h>
#include <vector>

namespace psvcd {

void PrintResp(std::vector<BYTE>& resp);

bool SetLinesIdle(FT_HANDLE ftHandle);

//this function is used to:
//send command on CMD line
//then poll CMD line till LOW
//then read responce on CMD line
//then write data packet on DAT0 line
//then poll DAT0 line till LOW
//then read N bytes on DAT0 line
//then poll DAT0 line till HIGH
//typical usage: CMD56
bool SendCMD_ReadResp_SendData(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp, const std::vector<BYTE>& packet, WORD waitSize = 10);

//this function is used to:
//send command on CMD line
//then poll CMD line till LOW
//then read responce on CMD line
//typical usage: any standard command
bool SendCMD_ReadResp(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp);

//this function is used to:
//send command on CMD line
//then poll CMD line till LOW
//then read responce on CMD line
//then poll DAT0 till HIGH
//typical usage: CMD6
bool SendCMD_ReadResp_Poll(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp);

//this function is used to:
//send command on CMD line
//then poll CMD line till LOW
//then read responce on CMD line
//then poll DAT0 till LOW
//then read N bytes
//typical usage: CMD8, CMD17
bool SendCMD_ReadResp_ReadData(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp, WORD dataSize);

//this function is used to:
//send command on CMD line
//then poll CMD line till LOW
//then read responce on CMD line
//then repeat M times: poll DAT0 till LOW and then read N bytes
//typical usage: CMD18
bool SendCMD_ReadResp_ReadDataMultiple(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp, WORD dataSize, DWORD times);

//this function is used to:
//send command on CMD line
//then hold CLK line for N bytes
//typical usage: CMD0, CMD7 (deselect), CMD15
bool SendCMD_Hold(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD nBytes);

};