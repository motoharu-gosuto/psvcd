#include <windows.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <array>
#include <iostream>

#include <ftd2xx.h>

#include <common/CardRoutines.h>
#include <common/FTDIInitialization.h>
#include <common/CRC.h>

#include "MMCCommands.h"
#include "CMD56Initialization.h"
#include "KirkClient.h"

std::array<BYTE, 0x20> block0 =  
{
      0xDD, 0x10, 0x25, 0x44, 0x15, 0x23, 0xFD, 0xC0, 
      0xF9, 0xE9, 0x15, 0x26, 0xDC, 0x2A, 0xE0, 0x84,
      0xA9, 0x03, 0xA2, 0x97, 0xD4, 0xBB, 0xF8, 0x52,
      0xD3, 0xD4, 0x94, 0x2C, 0x89, 0x03, 0xCC, 0x77,
};

#pragma pack(push, 1)

struct cmd56_packet_request_base
{
   //basic data
   uint8_t block0[0x20];
   uint32_t responseCode;
   uint32_t additionalDataSize0;
   uint32_t expectedRespSize;

   //additional data
   uint8_t code;
   uint8_t unk;
   uint8_t additionalDataSize1;
};

struct cmd56_packet_response_base
{
   uint32_t responseCode;
   uint32_t unk;
   uint16_t size; //big endian
   uint8_t errorCode;
};

//-------------------------

struct cmd56_packet1
{
   cmd56_packet_request_base data;
};

struct cmd56_packet2
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x10];
};

struct cmd56_packet3
{
   cmd56_packet_request_base data;
};

struct cmd56_packet4
{
   cmd56_packet_response_base data;
   
   uint8_t state0; //initialization status - shows if already initialized? - changes after sending packet 11 which has same structure
   uint8_t state1; //initialization status - shows if already initialized? - changes after sending packet 11 which has same structure
};

struct cmd56_packet5
{
   cmd56_packet_request_base data;
};

struct cmd56_packet6
{
   cmd56_packet_response_base data;
   
   uint8_t state0;
   uint8_t state1;
   
   uint16_t param0;
   uint16_t param1;
   uint16_t param2;

   uint8_t additionalData[0x20];
};

struct cmd56_packet7
{
   cmd56_packet_request_base data;
   uint16_t param0;
   uint8_t additionalData[0x10];
};

struct cmd56_packet8
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x20];
};

struct cmd56_packet9
{
   cmd56_packet_request_base data;
   uint8_t additionalData[0x30];
};

struct cmd56_packet10
{
   cmd56_packet_response_base data;
};

struct cmd56_packet11
{
   cmd56_packet_request_base data;
};

struct cmd56_packet12
{
   cmd56_packet_response_base data;
   
   uint8_t state0; //initialization status - shows if already initialized? - changes after sending packet 11 which has same structure
   uint8_t state1; //initialization status - shows if already initialized? - changes after sending packet 11 which has same structure
};

struct cmd56_packet13
{
   cmd56_packet_request_base data;
   uint8_t additionalData[0x10];
};

struct cmd56_packet14
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x40];
};

struct cmd56_packet15
{
   cmd56_packet_request_base data;
   uint8_t additionalData[0x30];
};

struct cmd56_packet16
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x40];
};

struct cmd56_packet17
{
   cmd56_packet_request_base data;
   uint8_t additionalData[0x30];
};

struct cmd56_packet18
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x40];
};

struct cmd56_packet19
{
   cmd56_packet_request_base data;
   uint8_t additionalData[0x10];
};

struct cmd56_packet20
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x50];
};

#pragma pack(pop)

template<int N>
void get_cmd56_p_base(const std::array<BYTE, N> bytes, std::vector<BYTE>& packet)
{
   packet.assign(bytes.begin(), bytes.end());
   while(packet.size() < 512)
      packet.push_back(0x00);

   uint16_t crc16 = psvcd::CRC_16(packet.data(), packet.size());

   packet.push_back(crc16 >> 8);
   packet.push_back(crc16 & 0x00FF);
}

void get_cmd56_p_base(const std::vector<BYTE>& bytes, std::vector<BYTE>& packet)
{
   packet.assign(bytes.begin(), bytes.end());
   while(packet.size() < 512)
      packet.push_back(0x00);

   uint16_t crc16 = psvcd::CRC_16(packet.data(), packet.size());

   packet.push_back(crc16 >> 8);
   packet.push_back(crc16 & 0x00FF);
}

//------------------------------------

int KirkGen10(SOCKET socket, std::array<BYTE, 0x10>& kirk_gen10_data)
{
   command_2_request cmd2;
   cmd2.command = PSVKIRK_COMMAND_GEN10;

   int iResult = send(socket, (const char*)&cmd2, sizeof(command_2_request), 0);
   if (iResult == SOCKET_ERROR) 
   {
      std::cout << "send failed with error: %d\n" << WSAGetLastError() << std::endl;
      closesocket(socket);
      WSACleanup();
      return -1;
   }

   command_2_response resp2;
   iResult = recv(socket, (char*)&resp2, sizeof(command_2_response), 0);
   if (iResult == SOCKET_ERROR) 
   {
      std::cout << "send failed with error: %d\n" << WSAGetLastError() << std::endl;
      closesocket(socket);
      WSACleanup();
      return -1;
   }

   if(resp2.base.command != PSVKIRK_COMMAND_GEN10 || resp2.base.vita_err < 0 || resp2.base.proxy_err < 0)
   {
      closesocket(socket);
      WSACleanup();
      return -1;
   }

   memcpy(kirk_gen10_data.data(), resp2.data, 0x10);

   return 0;
}

//------------------------------------

int SendKirkProxyCommand4(SOCKET socket, command_4_request* cmd4, command_4_response* resp4)
{
   int bytesToSend = sizeof(command_4_request);
   int bytesWereSend = 0;

   while(bytesToSend != bytesWereSend)
   {
      int iResult = send(socket, ((const char*)cmd4) + bytesWereSend, bytesToSend - bytesWereSend, 0);
      if (iResult == SOCKET_ERROR) 
      {
         std::cout << "send failed with error: %d\n" << WSAGetLastError() << std::endl;
         closesocket(socket);
         WSACleanup();
         return -1;
      }

      bytesWereSend = bytesWereSend + iResult;
   }

   int bytesToReceive = sizeof(command_4_response);
   int bytesWereReceived = 0;
   command_4_response* respcpy = resp4;

   while(bytesToReceive != bytesWereReceived)
   {
      int iResult = recv(socket, ((char*)respcpy) + bytesWereReceived, bytesToReceive - bytesWereReceived, 0);
      if (iResult == SOCKET_ERROR) 
      {
         std::cout << "send failed with error: %d\n" << WSAGetLastError() << std::endl;
         closesocket(socket);
         WSACleanup();
         return -1;
      }

      bytesWereReceived = bytesWereReceived + iResult;
   }

   if(resp4->base.command != PSVKIRK_COMMAND_KIRK || resp4->base.vita_err < 0 || resp4->base.proxy_err != 0)
   {
      closesocket(socket);
      WSACleanup();
      return -1;
   }

   return 0;
}

//Kirk services that are required are 1C and 1E
//Everything else is optional 
//Card can be initialized without other calls

int KirkSendCommand_1B(SOCKET socket, uint16_t param0, 
                        std::array<BYTE, 0x20>& packet6_data,
                        std::array<BYTE, 0x10>& packet7_data,
                        std::array<BYTE, 0x23>& packet8_data)
{
   command_4_request cmd4;
   cmd4.command = PSVKIRK_COMMAND_KIRK;
   cmd4.kirk_command = 0x1B;
   cmd4.size = sizeof(kirk_1B_input); //0x53
   cmd4.kirk_param = param0;

   kirk_1B_input input;
   memcpy(input.packet6, packet6_data.data(), 0x20);
   memcpy(input.packet7, packet7_data.data(), 0x10);
   memcpy(input.packet8, packet8_data.data(), 0x23);

   memset(cmd4.data, 0, 0x800);
   memcpy(cmd4.data, &input, sizeof(kirk_1B_input)); //0x53

   command_4_response resp4;

   return SendKirkProxyCommand4(socket, &cmd4, &resp4);
}

int KirkSendCommand_1C(SOCKET socket, uint16_t param0, 
                       std::array<BYTE, 0x20>& packet6_data,
                       std::array<BYTE, 0x23>& packet8_data,
                       std::array<BYTE, 0x33>& result_1c)
{
   command_4_request cmd4;
   cmd4.command = PSVKIRK_COMMAND_KIRK;
   cmd4.kirk_command = 0x1C;
   cmd4.size = sizeof(kirk_1C_input); //0x40
   cmd4.kirk_param = param0;

   kirk_1C_input input;
   memcpy(input.packet6, packet6_data.data(), 0x20);
   memcpy(input.packet8, packet8_data.data() + 0x03, 0x20);

   memset(cmd4.data, 0, 0x800);
   memcpy(cmd4.data, &input, sizeof(kirk_1C_input)); //0x40

   command_4_response resp4;
   
   if(SendKirkProxyCommand4(socket, &cmd4, &resp4) < 0)
      return -1;

   if(resp4.size != 0x33) // expected size
      return -1;

   memcpy(result_1c.data(), resp4.data, resp4.size);
   return 0;
}

int KirkSendCommand_1D(SOCKET socket, uint16_t param0,
                       std::array<BYTE, 0x20>& packet6_data,
                       std::array<BYTE, 0x33>& result_1c, 
                       std::array<BYTE, 0x10>& kirk_gen10_data1,
                       std::array<BYTE, 0x43>& packet14_data)
{
   command_4_request cmd4;
   cmd4.command = PSVKIRK_COMMAND_KIRK;
   cmd4.kirk_command = 0x1D;
   cmd4.size = sizeof(kirk_1D_input); //0xA3
   cmd4.kirk_param = param0;

   kirk_1D_input input;
   memcpy(input.packet6, packet6_data.data(), 0x20);
   memcpy(input.packet9, result_1c.data() + 0x03, 0x30);
   memcpy(input.packet13, kirk_gen10_data1.data(), 0x10);
   memcpy(input.packet14, packet14_data.data(), 0x43);

   memset(cmd4.data, 0, 0x800);
   memcpy(cmd4.data, &input, sizeof(kirk_1D_input)); //0xA3

   command_4_response resp4;
   
   if(SendKirkProxyCommand4(socket, &cmd4, &resp4) < 0)
      return -1;

   return 0;
}

int KirkSendCommand_1E(SOCKET socket, uint16_t param0, uint8_t param1,
                       std::array<BYTE, 0x20>& packet6_data,
                       std::array<BYTE, 0x33>& result_1c,
                       std::array<BYTE, 0x33>& result_1e)
{
   command_4_request cmd4;
   cmd4.command = PSVKIRK_COMMAND_KIRK;
   cmd4.kirk_command = 0x1E;
   cmd4.size = sizeof(kirk_1E_input); //0x51
   cmd4.kirk_param = param0;

   kirk_1E_input input;
   memcpy(input.packet6, packet6_data.data(), 0x20);
   memcpy(input.packet9, result_1c.data() + 0x03, 0x30);
   input.param = param1;

   memset(cmd4.data, 0, 0x800);
   memcpy(cmd4.data, &input, sizeof(kirk_1E_input)); //0x51

   command_4_response resp4;
   
   if(SendKirkProxyCommand4(socket, &cmd4, &resp4) < 0)
      return -1;

   if(resp4.size != 0x33) // expected size
      return -1;

   memcpy(result_1e.data(), resp4.data, resp4.size);
   return 0;
}

int KirkSendCommand_1F(SOCKET socket, uint16_t param0,
                       std::array<BYTE, 0x20>& packet6_data,
                       std::array<BYTE, 0x33>& result_1c, 
                       std::array<BYTE, 0x33>& result_1e2,
                       std::array<BYTE, 0x43>& packet16_data)
{
   command_4_request cmd4;
   cmd4.command = PSVKIRK_COMMAND_KIRK;
   cmd4.kirk_command = 0x1F;
   cmd4.size = sizeof(kirk_1F_input); //0xB3
   cmd4.kirk_param = param0;

   kirk_1F_input input;
   memcpy(input.packet6, packet6_data.data(), 0x20);
   memcpy(input.packet9, result_1c.data() + 0x03, 0x30);
   memcpy(input.packet15, result_1e2.data() + 0x03, 0x20);
   memcpy(input.packet16, packet16_data.data(), 0x43);

   memset(cmd4.data, 0, 0x800);
   memcpy(cmd4.data, &input, sizeof(kirk_1F_input)); //0x51

   command_4_response resp4;
   
   if(SendKirkProxyCommand4(socket, &cmd4, &resp4) < 0)
      return -1;

   if(resp4.size != 0x20) // expected size
      return -1;

   return 0;
}

int KirkSendCommand_20(SOCKET socket, uint16_t param0,
                       std::array<BYTE, 0x20>& packet6_data,
                       std::array<BYTE, 0x33>& result_1c, 
                       std::array<BYTE, 0x33>& result_1e3,
                       std::array<BYTE, 0x43>& packet18_data,
                       std::array<BYTE, 0x10>& kirk_gen10_data2,
                       std::array<BYTE, 0x53>& packet20_data)
{
   command_4_request cmd4;
   cmd4.command = PSVKIRK_COMMAND_KIRK;
   cmd4.kirk_command = 0x20;
   cmd4.size = sizeof(kirk_20_input); //0x116
   cmd4.kirk_param = param0;

   kirk_20_input input;
   memcpy(input.packet6, packet6_data.data(), 0x20);
   memcpy(input.packet9, result_1c.data() + 0x03, 0x30);
   memcpy(input.packet17, result_1e3.data() + 3, 0x20);
   memcpy(input.packet18, packet18_data.data(), 0x43);
   memcpy(input.packet19, kirk_gen10_data2.data(), 0x10);
   memcpy(input.packet20, packet20_data.data(), 0x53);

   memset(cmd4.data, 0, 0x800);
   memcpy(cmd4.data, &input, sizeof(kirk_20_input)); //0x116

   command_4_response resp4;
   
   if(SendKirkProxyCommand4(socket, &cmd4, &resp4) < 0)
      return -1;

   if(resp4.size != 0x34) // expected size
      return -1;

   return 0;
}

//------------------------------------

int SendPacket1(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet1 p1_raw;
   memcpy(p1_raw.data.block0, block0.data(), 0x20);
   p1_raw.data.responseCode = 0x31;
   p1_raw.data.additionalDataSize0 = 0x03;
   p1_raw.data.expectedRespSize = 0x13;
   p1_raw.data.code = 0xC4;
   p1_raw.data.unk = 0x00;
   p1_raw.data.additionalDataSize1 = 0x03;

   std::vector<BYTE> p1_bytes(sizeof(cmd56_packet1));
   memcpy(p1_bytes.data(), &p1_raw, sizeof(cmd56_packet1));

   std::vector<BYTE> p1_final;
   get_cmd56_p_base(p1_bytes, p1_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p1_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket2(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r2_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r2_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet2 r2_final;
   memcpy(&r2_final, r2_raw.data(), sizeof(cmd56_packet2));

   if(r2_final.data.responseCode != 0x31)
      return -1;

   if(r2_final.data.unk != 0x00)
      return -1;

   if(r2_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r2_final.data.size << 8) | (uint16_t)(r2_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x13) //not in original implementation
      return -1;

   return 0;
}

int SendPacket3(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet3 p3_raw;
   memcpy(p3_raw.data.block0, block0.data(), 0x20);
   p3_raw.data.responseCode = 0x23;
   p3_raw.data.additionalDataSize0 = 0x03;
   p3_raw.data.expectedRespSize = 0x05;
   p3_raw.data.code = 0xC2;
   p3_raw.data.unk = 0x00;
   p3_raw.data.additionalDataSize1 = 0x03;

   std::vector<BYTE> p3_bytes(sizeof(cmd56_packet3));
   memcpy(p3_bytes.data(), &p3_raw, sizeof(cmd56_packet3));

   std::vector<BYTE> p3_final;
   get_cmd56_p_base(p3_bytes, p3_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p3_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket4(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r4_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r4_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet4 r4_final;
   memcpy(&r4_final, r4_raw.data(), sizeof(cmd56_packet4));
   
   if(r4_final.data.responseCode != 0x23)
      return -1;

   if(r4_final.data.unk != 0x00)
      return -1;

   if(r4_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r4_final.data.size << 8) | (uint16_t)(r4_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x5) //not in original implementation
      return -1;

   //check initialization state
   if(r4_final.state0 == 0) //must be 0xFF
      return -1;

   if(r4_final.state1 != 0) //must be 0x00
      return -1;

   return 0;
}

int SendPacket5(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet5 p5_raw;
   memcpy(p5_raw.data.block0, block0.data(), 0x20);
   p5_raw.data.responseCode = 0x02;
   p5_raw.data.additionalDataSize0 = 0x03;
   p5_raw.data.expectedRespSize = 0x2B;
   p5_raw.data.code = 0xA1;
   p5_raw.data.unk = 0x00;
   p5_raw.data.additionalDataSize1 = 0x03;

   std::vector<BYTE> p5_bytes(sizeof(cmd56_packet5));
   memcpy(p5_bytes.data(), &p5_raw, sizeof(cmd56_packet5));

   std::vector<BYTE> p5_final;
   get_cmd56_p_base(p5_bytes, p5_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p5_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket6(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x20>& packet6_data, uint16_t& param0)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r6_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r6_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet6 r6_final;
   memcpy(&r6_final, r6_raw.data(), sizeof(cmd56_packet6));

   if(r6_final.data.responseCode != 0x02)
      return -1;

   if(r6_final.data.unk != 0x00)
      return -1;

   if(r6_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r6_final.data.size << 8) | (uint16_t)(r6_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x2B) //not in original implementation
      return -1;

   uint16_t param0_temp = (uint16_t)(r6_final.param0 << 8) | (uint16_t)(r6_final.param0 >> 8);
   uint16_t param1_temp = (uint16_t)(r6_final.param1 << 8) | (uint16_t)(r6_final.param1 >> 8); //not in original implementation
   uint16_t param2_temp = (uint16_t)(r6_final.param2 << 8) | (uint16_t)(r6_final.param2 >> 8); //not in original implementation

   if((param0_temp & (0x7FFF)) != 1)
      return -1;

   if(r6_final.state0 == 0x00) // must be 0xE0 - not in original implementation
      return -1;

   if(r6_final.state1 != 0) //not in original implementation
      return -1;

   memcpy(packet6_data.data(), r6_final.additionalData, 0x20);
   
   param0 = param0_temp;
   
   return 0;
}

int SendPacket7(FT_HANDLE ftHandle, WORD RCA, uint16_t param0, std::array<BYTE, 0x10>& kirk_gen10_data0)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet7 p7_raw;
   memcpy(p7_raw.data.block0, block0.data(), 0x20);
   p7_raw.data.responseCode = 0x03;
   p7_raw.data.additionalDataSize0 = 0x15;
   p7_raw.data.expectedRespSize = 0x23;
   p7_raw.data.code = 0xA2;
   p7_raw.data.unk = 0x00;
   p7_raw.data.additionalDataSize1 = 0x15;

   p7_raw.param0 = param0;
   p7_raw.param0 = (uint16_t)(p7_raw.param0 << 8) | (uint16_t)(p7_raw.param0 >> 8);
   memcpy(p7_raw.additionalData, kirk_gen10_data0.data(), 0x10);

   std::vector<BYTE> p7_bytes(sizeof(cmd56_packet7));
   memcpy(p7_bytes.data(), &p7_raw, sizeof(cmd56_packet7));

   std::vector<BYTE> p7_final;
   get_cmd56_p_base(p7_bytes, p7_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p7_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket8(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x23>& packet8_data)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r8_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r8_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet8 r8_final;
   memcpy(&r8_final, r8_raw.data(), sizeof(cmd56_packet8));
      
   if(r8_final.data.responseCode != 0x03)
      return -1;

   if(r8_final.data.unk != 0x00)
      return -1;

   if(r8_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r8_final.data.size << 8) | (uint16_t)(r8_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x23) //not in original implementation
      return -1;

   memcpy(packet8_data.data(), &r8_final.data.size, 0x23);

   return 0;
}

int SendPacket9(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x33>& result_1c)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet9 p9_raw;
   memcpy(p9_raw.data.block0, block0.data(), 0x20);
   p9_raw.data.responseCode = 0x05;
   p9_raw.data.additionalDataSize0 = 0x33;
   p9_raw.data.expectedRespSize = 0x03;
   p9_raw.data.code = result_1c[0];
   p9_raw.data.unk = result_1c[1];
   p9_raw.data.additionalDataSize1 = result_1c[2];

   memcpy(p9_raw.additionalData, result_1c.data() + 3, 0x30);

   std::vector<BYTE> p9_bytes(sizeof(cmd56_packet9));
   memcpy(p9_bytes.data(), &p9_raw, sizeof(cmd56_packet9));

   std::vector<BYTE> p9_final;
   get_cmd56_p_base(p9_bytes, p9_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p9_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket10(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r10_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r10_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet10 r10_final;
   memcpy(&r10_final, r10_raw.data(), sizeof(cmd56_packet10));
      
   if(r10_final.data.responseCode != 0x05)
      return -1;

   if(r10_final.data.unk != 0x00)
      return -1;

   if(r10_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r10_final.data.size << 8) | (uint16_t)(r10_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x3) //not in original implementation
      return -1;

   return 0;
}

int SendPacket11(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet11 p11_raw;
   memcpy(p11_raw.data.block0, block0.data(), 0x20);
   p11_raw.data.responseCode = 0x23;
   p11_raw.data.additionalDataSize0 = 0x03;
   p11_raw.data.expectedRespSize = 0x05;
   p11_raw.data.code = 0xC2;
   p11_raw.data.unk = 0x00;
   p11_raw.data.additionalDataSize1 = 0x03;

   std::vector<BYTE> p11_bytes(sizeof(cmd56_packet11));
   memcpy(p11_bytes.data(), &p11_raw, sizeof(cmd56_packet11));

   std::vector<BYTE> p11_final;
   get_cmd56_p_base(p11_bytes, p11_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p11_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket12(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r12_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r12_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet12 r12_final;
   memcpy(&r12_final, r12_raw.data(), sizeof(cmd56_packet12));
   
   if(r12_final.data.responseCode != 0x23)
      return -1;

   if(r12_final.data.unk != 0x00)
      return -1;

   if(r12_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r12_final.data.size << 8) | (uint16_t)(r12_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x5) //not in original implementation
      return -1;

   //check initialization state
   if(r12_final.state0 != 0) //must be 0x00
      return -1;

   if(r12_final.state1 != 0) //must be 0x00
      return -1;

   return 0;
}

int SendPacket13(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x10>& kirk_gen10_data1)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet13 p13_raw;
   memcpy(p13_raw.data.block0, block0.data(), 0x20);
   p13_raw.data.responseCode = 0x07;
   p13_raw.data.additionalDataSize0 = 0x13;
   p13_raw.data.expectedRespSize = 0x43;
   p13_raw.data.code = 0xA4;
   p13_raw.data.unk = 0x00;
   p13_raw.data.additionalDataSize1 = 0x13;

   memcpy(p13_raw.additionalData, kirk_gen10_data1.data(), 0x10);

   std::vector<BYTE> p13_bytes(sizeof(cmd56_packet13));
   memcpy(p13_bytes.data(), &p13_raw, sizeof(cmd56_packet13));

   std::vector<BYTE> p13_final;
   get_cmd56_p_base(p13_bytes, p13_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p13_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket14(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x43>& packet14_data)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r14_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r14_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet14 r14_final;
   memcpy(&r14_final, r14_raw.data(), sizeof(cmd56_packet14));
      
   if(r14_final.data.responseCode != 0x07)
      return -1;

   if(r14_final.data.unk != 0x00)
      return -1;

   if(r14_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r14_final.data.size << 8) | (uint16_t)(r14_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x43) //not in original implementation
      return -1;

   memcpy(packet14_data.data(), &r14_final.data.size, 0x43);

   return 0;
};

int SendPacket15(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x33>& result_1e2)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet15 p15_raw;
   memcpy(p15_raw.data.block0, block0.data(), 0x20);
   p15_raw.data.responseCode = 0x11;
   p15_raw.data.additionalDataSize0 = 0x33;
   p15_raw.data.expectedRespSize = 0x43;
   p15_raw.data.code = result_1e2[0];
   p15_raw.data.unk = result_1e2[1];
   p15_raw.data.additionalDataSize1 = result_1e2[2];

   memcpy(p15_raw.additionalData, result_1e2.data() + 3, 0x30);

   std::vector<BYTE> p15_bytes(sizeof(cmd56_packet15));
   memcpy(p15_bytes.data(), &p15_raw, sizeof(cmd56_packet15));

   std::vector<BYTE> p15_final;
   get_cmd56_p_base(p15_bytes, p15_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p15_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket16(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x43>& packet16_data)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r16_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r16_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet16 r16_final;
   memcpy(&r16_final, r16_raw.data(), sizeof(cmd56_packet16));
      
   if(r16_final.data.responseCode != 0x11)
      return -1;

   if(r16_final.data.unk != 0x00)
      return -1;

   if(r16_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r16_final.data.size << 8) | (uint16_t)(r16_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x43) //not in original implementation
      return -1;

   memcpy(packet16_data.data(), &r16_final.data.size, 0x43);

   return 0;
};

int SendPacket17(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x33>& result_1e3)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet17 p17_raw;
   memcpy(p17_raw.data.block0, block0.data(), 0x20);
   p17_raw.data.responseCode = 0x11;
   p17_raw.data.additionalDataSize0 = 0x33;
   p17_raw.data.expectedRespSize = 0x43;
   p17_raw.data.code = result_1e3[0];
   p17_raw.data.unk = result_1e3[1];
   p17_raw.data.additionalDataSize1 = result_1e3[2];

   memcpy(p17_raw.additionalData, result_1e3.data() + 3, 0x30);

   std::vector<BYTE> p17_bytes(sizeof(cmd56_packet17));
   memcpy(p17_bytes.data(), &p17_raw, sizeof(cmd56_packet17));

   std::vector<BYTE> p17_final;
   get_cmd56_p_base(p17_bytes, p17_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p17_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket18(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x43>& packet18_data)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r18_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r18_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet18 r18_final;
   memcpy(&r18_final, r18_raw.data(), sizeof(cmd56_packet18));
      
   if(r18_final.data.responseCode != 0x11)
      return -1;

   if(r18_final.data.unk != 0x00)
      return -1;

   if(r18_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r18_final.data.size << 8) | (uint16_t)(r18_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x43) //not in original implementation
      return -1;

   memcpy(packet18_data.data(), &r18_final.data.size, 0x43);

   return 0;
};

int SendPacket19(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x10>& kirk_gen10_data2)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   cmd56_packet19 p19_raw;
   memcpy(p19_raw.data.block0, block0.data(), 0x20);
   p19_raw.data.responseCode = 0x19;
   p19_raw.data.additionalDataSize0 = 0x13;
   p19_raw.data.expectedRespSize = 0x53;
   p19_raw.data.code = 0xC1;
   p19_raw.data.unk = 0x00;
   p19_raw.data.additionalDataSize1 = 0x13;

   memcpy(p19_raw.additionalData, kirk_gen10_data2.data(), 0x10);

   std::vector<BYTE> p19_bytes(sizeof(cmd56_packet19));
   memcpy(p19_bytes.data(), &p19_raw, sizeof(cmd56_packet19));

   std::vector<BYTE> p19_final;
   get_cmd56_p_base(p19_bytes, p19_final);

   if(!psvcd::CMD56_MMC_1_WRITE(ftHandle, p19_final))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

int ReceivePacket20(FT_HANDLE ftHandle, WORD RCA, std::array<BYTE, 0x53>& packet20_data)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));

   std::vector<BYTE> r20_raw;
   if(!psvcd::CMD56_MMC_1_READ(ftHandle, r20_raw))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return - 1;
   }

   cmd56_packet20 r20_final;
   memcpy(&r20_final, r20_raw.data(), sizeof(cmd56_packet20));
      
   if(r20_final.data.responseCode != 0x19)
      return -1;

   if(r20_final.data.unk != 0x00)
      return -1;

   if(r20_final.data.errorCode != 0x00)
      return -1;

   uint16_t dataSize = (uint16_t)(r20_final.data.size << 8) | (uint16_t)(r20_final.data.size >> 8); //not in original implementation
   if(dataSize != 0x53) //not in original implementation
      return -1;

   memcpy(packet20_data.data(), &r20_final.data.size, 0x53);

   return 0;
};

int SendCMD17(FT_HANDLE ftHandle, WORD RCA)
{
   std::this_thread::sleep_for(std::chrono::milliseconds(40));
   
   std::vector<BYTE> respData;
   if(!psvcd::CMD17_MMC(ftHandle, 0x0, 0x0, 0x0, 0x00, respData))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return -1;
   }

   return 0;
}

void psvcd::CMD56Initialization(FT_HANDLE ftHandle, WORD RCA)
{
   if(SendPacket1(ftHandle, RCA) < 0)
      return;

   if(ReceivePacket2(ftHandle, RCA) < 0)
      return;

   if(SendPacket3(ftHandle, RCA) < 0)
      return;

   if(ReceivePacket4(ftHandle, RCA) < 0)
      return;

   if(SendPacket5(ftHandle, RCA) < 0)
      return;

   std::array<BYTE, 0x20> packet6_data;
   memset(packet6_data.data(), 0, packet6_data.size());

   uint16_t param0 = 0;

   if(ReceivePacket6(ftHandle, RCA, packet6_data, param0) < 0)
      return;

   SOCKET kirk_socket = 0;
   if(initialize_kirk_proxy_connection(kirk_socket) < 0)
      return;

   std::array<BYTE, 0x10> kirk_gen10_data0;
   memset(kirk_gen10_data0.data(), 0, kirk_gen10_data0.size());

   if(KirkGen10(kirk_socket, kirk_gen10_data0) < 0)
      return;

   kirk_gen10_data0[0] = (kirk_gen10_data0[0] | (~0x7F));

   if(SendPacket7(ftHandle, RCA, param0, kirk_gen10_data0))
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x23> packet8_data;
   memset(packet8_data.data(), 0, packet8_data.size());

   if(ReceivePacket8(ftHandle, RCA, packet8_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(KirkSendCommand_1B(kirk_socket, param0, packet6_data, kirk_gen10_data0, packet8_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x33> result_1c;
   memset(result_1c.data(), 0, result_1c.size());

   if(KirkSendCommand_1C(kirk_socket, param0, packet6_data, packet8_data, result_1c) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(SendPacket9(ftHandle, RCA, result_1c) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(ReceivePacket10(ftHandle, RCA) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(SendPacket11(ftHandle, RCA) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(ReceivePacket12(ftHandle, RCA) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x10> kirk_gen10_data1;
   memset(kirk_gen10_data1.data(), 0, kirk_gen10_data1.size());

   if(KirkGen10(kirk_socket, kirk_gen10_data1) < 0)
      return;

   kirk_gen10_data1[0] = (kirk_gen10_data1[0] | (~0x7F));

   if(SendPacket13(ftHandle, RCA, kirk_gen10_data1) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x43> packet14_data;
   memset(packet14_data.data(), 0, 0x43);

   if(ReceivePacket14(ftHandle, RCA, packet14_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   //currently does not work
   /*
   if(KirkSendCommand_1D(kirk_socket, param0, packet6_data, result_1c, kirk_gen10_data1, packet14_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }
   */

   std::array<BYTE, 0x33> result_1e2;
   memset(result_1e2.data(), 0, result_1e2.size());

   if(KirkSendCommand_1E(kirk_socket, param0, 0x02, packet6_data, result_1c, result_1e2) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(SendPacket15(ftHandle, RCA, result_1e2) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x43> packet16_data;
   memset(packet16_data.data(), 0, 0x43);

   if(ReceivePacket16(ftHandle, RCA, packet16_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   //currently does not work
   /*
   if(KirkSendCommand_1F(kirk_socket, param0, packet6_data, result_1c, result_1e2, packet16_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }
   */

   std::array<BYTE, 0x33> result_1e3;
   memset(result_1e3.data(), 0, result_1e3.size());

   if(KirkSendCommand_1E(kirk_socket, param0, 0x03, packet6_data, result_1c, result_1e3) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   if(SendPacket17(ftHandle, RCA, result_1e3) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x43> packet18_data;
   memset(packet18_data.data(), 0, 0x43);

   if(ReceivePacket18(ftHandle, RCA, packet18_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x10> kirk_gen10_data2;
   memset(kirk_gen10_data2.data(), 0, kirk_gen10_data2.size());

   if(KirkGen10(kirk_socket, kirk_gen10_data2) < 0)
      return;

   kirk_gen10_data2[0] = (kirk_gen10_data2[0] | (~0x7F));

   if(SendPacket19(ftHandle, RCA, kirk_gen10_data2) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   std::array<BYTE, 0x53> packet20_data;
   memset(packet20_data.data(), 0, 0x53);

   if(ReceivePacket20(ftHandle, RCA, packet20_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   //currently does not work
   /*
   if(KirkSendCommand_20(kirk_socket, param0, packet6_data, result_1c, result_1e3, packet18_data, kirk_gen10_data2, packet20_data) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }
   */

   if(deinitialize_kirk_proxy_connection(kirk_socket) < 0)
      return;

   SendCMD17(ftHandle, RCA);

   return;
}