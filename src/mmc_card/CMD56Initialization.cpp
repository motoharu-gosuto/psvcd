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

struct cmd56_packet_reqest_base
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
   cmd56_packet_reqest_base data;
};

struct cmd56_packet2
{
   cmd56_packet_response_base data;
   uint8_t additionalData[0x10];
};

struct cmd56_packet3
{
   cmd56_packet_reqest_base data;
};

struct cmd56_packet4
{
   cmd56_packet_response_base data;
   
   uint8_t state0; //initialization status - shows if already initialized? - changes after sending packet 11 which has same structure
   uint8_t state1; //initialization status - shows if already initialized? - changes after sending packet 11 which has same structure
};

struct cmd56_packet5
{
   cmd56_packet_reqest_base data;
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
   cmd56_packet_reqest_base data;
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
   cmd56_packet_reqest_base data;
   uint8_t additionalData[0x30];
};

struct cmd56_packet10
{
   cmd56_packet_response_base data;
};

//------------------------------------

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
   if(iResult != sizeof(command_2_response))
   {
      if(iResult == 0)
         std::cout << "connection closed\n" << std::endl;
      else
         std::cout << "failed to read response\n" << std::endl;
      
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

   while(bytesToReceive != bytesWereReceived)
   {
      int iResult = recv(socket, ((char*)&resp4) + bytesWereReceived, bytesToReceive - bytesWereReceived, 0);
      if(iResult != sizeof(command_4_response))
      {
         if(iResult == 0)
            std::cout << "connection closed\n" << std::endl;
         else
            std::cout << "failed to read response\n" << std::endl;
      
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

   return SendKirkProxyCommand4(socket, &cmd4, &resp4);
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

   if(!ReceivePacket10(ftHandle, RCA) < 0)
   {
      deinitialize_kirk_proxy_connection(kirk_socket);
      return;
   }

   //the rest of the code - packets 11 to 20 go here

   if(deinitialize_kirk_proxy_connection(kirk_socket) < 0)
      return;

   /*
   std::this_thread::sleep_for(std::chrono::milliseconds(40));
   
   std::vector<BYTE> respData;
   if(!psvcd::CMD17_MMC(ftHandle, 0x0, 0x0, 0x0, 0x00, respData))
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(40));
      psvcd::CMD15_MMC(ftHandle, RCA);
      return;
   }
   */

   return;
}