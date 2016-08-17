#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

#include "CardRoutines.h"
#include "FTDICommon.h"
#include "FTDIInitialization.h"
#include "CRC.h"

#ifdef MMC_PINOUT
#include "MMCPins.h"
#elif SD_PINOUT
#include "SDPins.h"
#else
#error Pinout is not defined
#endif

#define FTDI_WRITE_WAIT_DELAY 2

void psvcd::PrintResp(std::vector<BYTE>& resp)
{
   for(size_t i = 0; i < resp.size(); i++)
   {
      std::cout << std::hex << (DWORD)resp[i] << " ";
   }
   std::cout << std::endl;
}

BYTE AcBusValue(BYTE mux_adr, BYTE demux_adr, BYTE read_write)
{
   //     AC7              AC6          AC5, AC4, AC3    AC2, AC1, AC0
  return (0x80) | (read_write << 6) | (demux_adr << 3) | (mux_adr);
}

bool psvcd::SetLinesIdle(FT_HANDLE ftHandle)
{
   DWORD dwNumBytesToSend = 0;
   BYTE OutputBuffer[20]; //TODO: Implement as dynamic buffer

   // AD0 - CLK
   // AD1 - DATA OUT
   // AD2 - DATA IN
   // AD3 - not used
   // AD4 - not used
   // AD5 - WAIT
   // AD6 - not used
   // AD7 - not used

   //AC0, AC1, AC2 - address mux (output)
   //AC3, AC4, AC5 - address demux (output)
   //AC6 - read/write (output)
   //AC7 - not used in self powered mode

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction and data
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set 7 lines to high level and SCL to low level // 1111 1110
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB; // direction: 1101 1011
                                                  // AD0, AD1 - out
                                                  // AD2      - in
                                                  // AD3, AD4 - out
                                                  // AD5 - in
                                                  // AD6, AD7 - out
   
   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F; // direction: 0111 1111
                                                  // AC0, AC1, AC2 - out (address mux)
                                                  // AC3, AC4, AC5 - out (address demux)
                                                  // AC6 - out - read/write
                                                  // AC7 - in (not used but should be input I guess)  
   //Send off the commands
   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));
   
   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   return true;
}

bool WaitReadByte(FT_HANDLE ftHandle, std::vector<BYTE>& dwDataGet, bool waitForData, DWORD dataSize)
{
   const DWORD CounterTimeout = 5000;

   DWORD dwNumInputBuffer = 0;
   DWORD ReadTimeoutCounter = 0;
   FT_STATUS ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Get number of bytes in the input buffer

   DWORD reqSize = waitForData ? dataSize : 1;

   while ((dwNumInputBuffer < reqSize) && (ftStatus == FT_OK) && (ReadTimeoutCounter < CounterTimeout))
   {
      // Sit in this loop until
      // (1) we receive the one byte expected
      // or (2) a hardware error occurs causing the GetQueueStatus to return an error code
      // or (3) we have checked 5000 times and the expected byte is not coming
      ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Get # bytes in buffer
      ReadTimeoutCounter++;

      //looks like this delay is needed. most likely polling can not be done continuously

      std::this_thread::sleep_for(std::chrono::milliseconds(1)); // short delay
   }

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to GetQueueStatus" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   if(ReadTimeoutCounter >= CounterTimeout)
   {
      std::cout << "GetQueueStatus timed out" << std::endl;
      return false;
   }

   // If loop above exited due to the byte coming back (not an error code and not a timeout)
   // then read the byte available and return True to indicate success

   char* InputBuffer = new char[dwNumInputBuffer];
   DWORD dwNumBytesRead = 0;

   ftStatus = FT_Read(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Read" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      delete [] InputBuffer;
      return false;
   }

   if(dwNumInputBuffer != dwNumBytesRead)
   {
      std::cout << "Failed to Read" << std::endl;
      delete [] InputBuffer;
      return false;
   }

   for(DWORD i = 0; i < dwNumBytesRead; i++)
   {
      dwDataGet.push_back(InputBuffer[i]); // store the data read
   }

   delete [] InputBuffer;
   
   return true;
}

bool psvcd::SendCMD_ReadResp_SendData(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp, const std::vector<BYTE>& packet, WORD waitSize)
{
   DWORD dwNumBytesToSend = 0;
   
   //TODO: Implement as dynamic buffer
   BYTE OutputBuffer[650]; //514 byte packet + some more space

   //============= ENTER WRITE MODE================

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, WRITE_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= SEND COMMAND ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06; //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFF; // sometimes it happens so that clock goes before data (not sure what is this issue. maybe bug in driver?)
                                                  // anyhow - prepending with FF seems to help in this situation. BTW PS Vita does the same thing.
                                                  // it sends around 4-5 clocks before sending real data
   OutputBuffer[dwNumBytesToSend++] = cmd; 
   OutputBuffer[dwNumBytesToSend++] = a0;
   OutputBuffer[dwNumBytesToSend++] = a1;
   OutputBuffer[dwNumBytesToSend++] = a2;
   OutputBuffer[dwNumBytesToSend++] = a3;
   OutputBuffer[dwNumBytesToSend++] = crc;

   //============ WAIT FOR RESPONSE ON CMD LINE =============

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes Low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   //=========== READ RESPONSE ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x22; //clock in data bits on rising edge
                                                  //we need to clock 7 more bits because 
                                                  //first bit will be clocked by 0x95 command 
                                                  //that waits for CMD line to go low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06;

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(respSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((respSize >> 0x08) & 0xFF); //LengthH

   //============= WRITE DATA PACKET ===============

   //select dat0 line with multiplexer
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_DAT0_ADDRESS, CARD_DAT0_ADDRESS, WRITE_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //clock out START BIT
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x02; // 1 + 512 + 2 - 1 = 514
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x02; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Start bit

   //send data
   for(size_t i = 0; i < packet.size(); i++)
      OutputBuffer[dwNumBytesToSend++] = packet[i];

   //============= WAIT FOR DAT0 LINE =====================

   //by some unknown reason - after gen_cmd is send to the card
   //card drives dat0 line down for around one 512 byte dataframe
   //during this time commands may not be sent

   //TODO: Figure out what kind of data is that

   //select DAT0
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_DAT0_ADDRESS, CARD_DAT0_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   //read some stuff from line (not sure how much)

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(waitSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((waitSize >> 0x08) & 0xFF); //LengthH

   //Poll with no data transfer until GPIOL1 goes high
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x94;
   
   //============= ENTER IDLE STATE =======================

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   11111110
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB; 

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= REQUEST ANSWER IMMEDIATELY =============

   // This command then tells the MPSSE to send any results gathered back immediately
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x87; // Send answer back immediate command

   //============= EXECUTE COMMANDS =======================

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer above
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   bool res = WaitReadByte(ftHandle, resp, true, waitSize);

   //fix up first byte - looks like unsampled bits are set to 1
   if(resp.size() > 0)
   {
      resp[0] = resp[0] & 0x7F;
   }

   return res;
}

bool psvcd::SendCMD_ReadResp(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp)
{
   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[60];  //TODO: Implement as dynamic buffer

   //============= ENTER WRITE MODE================

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, WRITE_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= SEND COMMAND ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06; //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFF; // sometimes it happens so that clock goes before data (not sure what is this issue. maybe bug in driver?)
                                                  // anyhow - prepending with FF seems to help in this situation. BTW PS Vita does the same thing.
                                                  // it sends around 4-5 clocks before sending real data
   OutputBuffer[dwNumBytesToSend++] = cmd; 
   OutputBuffer[dwNumBytesToSend++] = a0;
   OutputBuffer[dwNumBytesToSend++] = a1;
   OutputBuffer[dwNumBytesToSend++] = a2;
   OutputBuffer[dwNumBytesToSend++] = a3;
   OutputBuffer[dwNumBytesToSend++] = crc;

   //============ WAIT FOR RESPONSE ON CMD LINE =============

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes Low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   //=========== READ RESPONSE ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x22; //clock in data bits on rising edge
                                                  //we need to clock 7 more bits because 
                                                  //first bit will be clocked by 0x95 command 
                                                  //that waits for CMD line to go low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06;

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(respSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((respSize >> 0x08) & 0xFF); //LengthH

   //============= ENTER IDLE STATE =======================

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   11111110
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB; 

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= REQUEST ANSWER IMMEDIATELY =============

   // This command then tells the MPSSE to send any results gathered back immediately
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x87; // Send answer back immediate command

   //============= EXECUTE COMMANDS =======================

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer above
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   bool res = WaitReadByte(ftHandle, resp, false, 0);

   //fix up first byte - looks like unsampled bits are set to 1
   if(resp.size() > 0)
   {
      resp[0] = resp[0] & 0x7F;
   }

   return res;
}

bool psvcd::SendCMD_ReadResp_Poll(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp)
{
   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[60];  //TODO: Implement as dynamic buffer

   //============= ENTER WRITE MODE================

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, WRITE_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= SEND COMMAND ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06; //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFF; // sometimes it happens so that clock goes before data (not sure what is this issue. maybe bug in driver?)
                                                  // anyhow - prepending with FF seems to help in this situation. BTW PS Vita does the same thing.
                                                  // it sends around 4-5 clocks before sending real data
   OutputBuffer[dwNumBytesToSend++] = cmd; 
   OutputBuffer[dwNumBytesToSend++] = a0;
   OutputBuffer[dwNumBytesToSend++] = a1;
   OutputBuffer[dwNumBytesToSend++] = a2;
   OutputBuffer[dwNumBytesToSend++] = a3;
   OutputBuffer[dwNumBytesToSend++] = crc;

   //============ WAIT FOR RESPONSE ON CMD LINE =============

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes Low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   //=========== READ RESPONSE ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x22; //clock in data bits on rising edge
                                                  //we need to clock 7 more bits because 
                                                  //first bit will be clocked by 0x95 command 
                                                  //that waits for CMD line to go low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06;

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(respSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((respSize >> 0x08) & 0xFF); //LengthH

   //============= WAIT TILL DAT0 GOES HIGH ===============

   //select DAT0
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_DAT0_ADDRESS, CARD_DAT0_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes high
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x94;

   //============= ENTER IDLE STATE =======================

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   11111110
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB; 

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= REQUEST ANSWER IMMEDIATELY =============

   // This command then tells the MPSSE to send any results gathered back immediately
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x87; // Send answer back immediate command

   //============= EXECUTE COMMANDS =======================

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer above
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   bool res = WaitReadByte(ftHandle, resp, false, 0);

   //fix up first byte - looks like unsampled bits are set to 1
   if(resp.size() > 0)
   {
      resp[0] = resp[0] & 0x7F;
   }

   return res;
}

bool psvcd::SendCMD_ReadResp_ReadData(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp, WORD dataSize)
{
   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[60];  //TODO: Implement as dynamic buffer

   //============= ENTER WRITE MODE================

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, WRITE_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= SEND COMMAND ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06; //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFF; // sometimes it happens so that clock goes before data (not sure what is this issue. maybe bug in driver?)
                                                  // anyhow - prepending with FF seems to help in this situation. BTW PS Vita does the same thing.
                                                  // it sends around 4-5 clocks before sending real data
   OutputBuffer[dwNumBytesToSend++] = cmd; 
   OutputBuffer[dwNumBytesToSend++] = a0;
   OutputBuffer[dwNumBytesToSend++] = a1;
   OutputBuffer[dwNumBytesToSend++] = a2;
   OutputBuffer[dwNumBytesToSend++] = a3;
   OutputBuffer[dwNumBytesToSend++] = crc;

   //============ WAIT FOR RESPONSE ON CMD LINE =============

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes Low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   //=========== READ RESPONSE ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x22; //clock in data bits on rising edge
                                                  //we need to clock 7 more bits because 
                                                  //first bit will be clocked by 0x95 command 
                                                  //that waits for CMD line to go low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06;

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(respSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((respSize >> 0x08) & 0xFF); //LengthH

   //============= WAIT TILL DAT0 GOES LOW ===============

   //select DAT0
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_DAT0_ADDRESS, CARD_DAT0_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(dataSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((dataSize >> 0x08) & 0xFF); //LengthH
   
   //============= ENTER IDLE STATE =======================

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   11111110
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB; 

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= REQUEST ANSWER IMMEDIATELY =============

   // This command then tells the MPSSE to send any results gathered back immediately
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x87; // Send answer back immediate command

   //============= EXECUTE COMMANDS =======================

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer above
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   bool res = WaitReadByte(ftHandle, resp, true, dataSize);

   //fix up first byte - looks like unsampled bits are set to 1
   if(resp.size() > 0)
   {
      resp[0] = resp[0] & 0x7F;
   }

   return res;
}

bool psvcd::SendCMD_ReadResp_ReadDataMultiple(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD respSize, std::vector<BYTE>& resp, WORD dataSize, DWORD times)
{
   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[400];  //TODO: Implement as dynamic buffer

   //============= ENTER WRITE MODE================

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, WRITE_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= SEND COMMAND ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06; //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFF; // sometimes it happens so that clock goes before data (not sure what is this issue. maybe bug in driver?)
                                                  // anyhow - prepending with FF seems to help in this situation. BTW PS Vita does the same thing.
                                                  // it sends around 4-5 clocks before sending real data
   OutputBuffer[dwNumBytesToSend++] = cmd; 
   OutputBuffer[dwNumBytesToSend++] = a0;
   OutputBuffer[dwNumBytesToSend++] = a1;
   OutputBuffer[dwNumBytesToSend++] = a2;
   OutputBuffer[dwNumBytesToSend++] = a3;
   OutputBuffer[dwNumBytesToSend++] = crc;

   //============ WAIT FOR RESPONSE ON CMD LINE =============

   //select cmd line with multiplexer - will be waiting for this line
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //Poll with no data transfer until GPIOL1 goes Low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;

   //=========== READ RESPONSE ================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x22; //clock in data bits on rising edge
                                                  //we need to clock 7 more bits because 
                                                  //first bit will be clocked by 0x95 command 
                                                  //that waits for CMD line to go low
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06;

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(respSize & 0xFF);    // LengthL
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((respSize >> 0x08) & 0xFF); //LengthH

   //============= WAIT TILL DAT0 GOES HIGH ===============

   //select DAT0
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_DAT0_ADDRESS, CARD_DAT0_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;
      
   for(DWORD i = 0; i < times; i++)
   {
      //Poll with no data transfer until GPIOL1 goes low
      OutputBuffer[dwNumBytesToSend++] = (BYTE)0x95;
      
      OutputBuffer[dwNumBytesToSend++] = (BYTE)0x20; // Command: clock data byte in on clk rising edge
      OutputBuffer[dwNumBytesToSend++] = (BYTE)(dataSize & 0xFF);    // LengthL
      OutputBuffer[dwNumBytesToSend++] = (BYTE)((dataSize >> 0x08) & 0xFF); //LengthH
   }

   //============= ENTER IDLE STATE =======================

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   11111110
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB; 

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //set line values
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= REQUEST ANSWER IMMEDIATELY =============

   // This command then tells the MPSSE to send any results gathered back immediately
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x87; // Send answer back immediate command

   //============= EXECUTE COMMANDS =======================

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer above
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   bool res = WaitReadByte(ftHandle, resp, true, (dataSize + 1) * times);

   //fix up first byte - looks like unsampled bits are set to 1
   if(resp.size() > 0)
   {
      resp[0] = resp[0] & 0x7F;
   }

   return res;
}

bool psvcd::SendCMD_Hold(FT_HANDLE ftHandle, BYTE cmd, BYTE a0, BYTE a1, BYTE a2, BYTE a3, BYTE crc, WORD nBytes)
{
   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[60];  //TODO: Implement as dynamic buffer

   //=============== ENTER WRITE MODE ==================

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, WRITE_MODE); //mux cmd line, demux cmd line, mode - read
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   //============= SEND COMMAND ========================

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x11; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x06; //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; // Data length of 0x0000 means clock out 1 byte
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFF; // sometimes it happens so that clock goes before data (not sure what is this issue. maybe bug in driver?)
                                                  // anyhow - prepending with FF seems to help in this situation. BTW PS Vita does the same thing.
                                                  // it sends around 4-5 clocks before sending real data
   OutputBuffer[dwNumBytesToSend++] = cmd; 
   OutputBuffer[dwNumBytesToSend++] = a0;
   OutputBuffer[dwNumBytesToSend++] = a1;
   OutputBuffer[dwNumBytesToSend++] = a2;
   OutputBuffer[dwNumBytesToSend++] = a3;
   OutputBuffer[dwNumBytesToSend++] = crc;

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x8F; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(nBytes & 0xFF); //
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((nBytes >> 0x08) & 0xFF); // Data length of 0x0000 means clock out 1 byte     

   //============ RETURN TO READ MODE ===================

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB;

   //Enter idle state on ACbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x82; 
   OutputBuffer[dwNumBytesToSend++] = AcBusValue(CARD_CMD_ADDRESS, CARD_CMD_ADDRESS, READ_MODE); //mux cmd line, demux cmd line, mode - read
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F;

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer above
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to Write" << std::endl;
      return false;
   }

   return true;
}