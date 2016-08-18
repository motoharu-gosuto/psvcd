#include <iostream>
#include <chrono>
#include <thread>

#include "FTDICommon.h"
#include "FTDIInitialization.h"

#define FTDI_WRITE_WAIT_DELAY 2

bool CustomPurge(FT_HANDLE ftHandle)
{
   //Purge USB receive buffer first by reading out all old data from FT232H receive buffer

   DWORD dwNumBytesToRead = 0;

   // Get the number of bytes in the FT232H receive buffer
   FT_STATUS ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
   
   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to purge device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   if(dwNumBytesToRead <= 0)
      return true;

   DWORD dwNumBytesRead = 0;
   char* buffer = new char[dwNumBytesToRead];

   //Read out the data from FT232H receive buffer
   ftStatus = FT_Read(ftHandle, buffer, dwNumBytesToRead, &dwNumBytesRead); 

   delete [] buffer;

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to purge device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
   
   if(dwNumBytesRead != dwNumBytesToRead)
   {
      std::cout << "Failed to purge device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   return true;
}

bool psvcd::ConfigureFTDIPort(FT_HANDLE ftHandle)
{
   if(!ResetDevice(ftHandle))
      return false;

   if(!CustomPurge(ftHandle))
      return false;

   if(!SetUSBParameters(ftHandle, 65536, 65535)) //Set USB request transfer sizes to 64K
      return false;

   if(!SetChars(ftHandle, false, 0, false, 0)) //Disable event and error characters
      return false;
   
   if(!SetTimeouts(ftHandle, 0, 5000)) //Sets the read and write timeouts in milliseconds
      return false;
   
   if(!SetLatencyTimer(ftHandle, 1)) //Set the latency timer to 1mS (default is 16mS)
      return false;

   if(!SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0x0, 0x0)) //Turn on flow control to synchronize IN requests
      return false;

   if(!SetBitMode(ftHandle, 0x00, 0x00)) //Reset controller
      return false;

   if(!SetBitMode(ftHandle, 0x00, FT_BITMODE_MPSSE)) //Enable MPSSE mode
      return false;

   // Wait for all the USB stuff to complete and work
   std::this_thread::sleep_for(std::chrono::milliseconds(50));

   return true;
}

bool EnableLoopback(FT_HANDLE ftHandle)
{
   DWORD dwNumBytesToSend = 0;
   BYTE byOutputBuffer[20];  //TODO: Implement as dynamic buffer

   // Enable internal loop-back
   byOutputBuffer[dwNumBytesToSend++] = (BYTE)0x84;

   DWORD dwNumBytesSent = 0;

   // Enable loopback
   // Send off the loopback command
   FT_STATUS ftStatus = FT_Write(ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to enable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
   
   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to enable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   DWORD dwNumBytesToRead = 0;

   // Check the receive buffer - it should be empty
   ftStatus = ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to enable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   // Get the number of bytes in
   // the FT232H receive buffer
   if (dwNumBytesToRead != 0)
   {
      std::cout << "Failed to enable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   return true;
}

bool DisableLoopback(FT_HANDLE ftHandle)
{
   DWORD dwNumBytesToSend = 0;
   BYTE byOutputBuffer[20];  //TODO: Implement as dynamic buffer
   DWORD dwNumBytesSent = 0;

    // Disable internal loop-back
   byOutputBuffer[dwNumBytesToSend++] = (BYTE)0x85;
   
   // Disable loopback
   // Send off the loopback command
   FT_STATUS ftStatus = FT_Write(ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to disable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
   
   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Failed to disable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   DWORD dwNumBytesToRead = 0;

   // Check the receive buffer - it should be empty
   ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);
   
   // Get the number of bytes in
   // the FT232H receive buffer
   if (dwNumBytesToRead != 0)
   {
      std::cout << "Failed to disable loopback device" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }
   
   return true;
}

bool SyncMPSSE(FT_HANDLE ftHandle, BYTE command)
{
   const DWORD CounterTimeout = 500;

   // Synchronise the MPSSE by sending bad command AA to it   
   
   DWORD dwNumBytesToSend = 0;
   DWORD dwNumBytesSent = 0;
   BYTE OutputBuffer[20];  //TODO: Implement as dynamic buffer

   OutputBuffer[dwNumBytesToSend++] = command; // Add an invalid command 0xAA
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); // Send to FT232H

   // Wait for data to be transmitted and status to be returned by the device driver - see latency timer
   std::this_thread::sleep_for(std::chrono::milliseconds(FTDI_WRITE_WAIT_DELAY));

   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Write" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   } 

   // Check if the bytes were sent off OK
   if(dwNumBytesToSend != dwNumBytesSent)
   {
      std::cout << "Write timed out" << std::endl;
      return false;
   }

   // Now read the response from the FT232H. It should return error code 0xFA
   // followed by the actual bad command 0xAA
   DWORD dwNumInputBuffer = 0;
   DWORD ReadTimeoutCounter = 0;

   ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Get # bytes in input buffer
   while ((dwNumInputBuffer < 2) && (ftStatus == FT_OK) && (ReadTimeoutCounter < CounterTimeout))
   {
      // Sit in this loop until
      // (1) we receive the two bytes expected
      // or (2) a hardware error occurs causing the GetQueueStatus to return an error code
      // or (3) we have checked 500 times and the expected byte is not coming
      ftStatus = FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Check queue status
      ReadTimeoutCounter++;
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

   // If the loop above exited due to the byte coming back (not an error code and not a timeout)
   // then read the bytes available and check for the error code followed by the invalid character

   char InputBuffer[20];
   DWORD dwNumBytesRead = 0;

   ftStatus = FT_Read(ftHandle, &InputBuffer, dwNumInputBuffer, &dwNumBytesRead); // Now read data
   
   if (ftStatus != FT_OK) 
   {
      std::cout << "Failed to Read" << std::endl;
      std::cout << "Error: " << psvcd::ErrorCodeToString(ftStatus) << std::endl;
      return false;
   }

   if(dwNumInputBuffer != dwNumBytesRead)
   {
      std::cout << "Failed to Read" << std::endl;
      return false;
   }

   // Check if we have two consecutive bytes in the buffer with value 0xFA and 0xAA
   bool bCommandEchod = false;
   for(DWORD dwCount = 0; dwCount < dwNumBytesRead - 1; dwCount++) 
   {
      if ((BYTE(InputBuffer[dwCount]) == BYTE(0xFA)) && (BYTE(InputBuffer[dwCount+1]) == BYTE(command)))
      {
         bCommandEchod = true;
         break;
      }
   }

   // If the device did not respond correctly, display error message and exit.
   if (!bCommandEchod)
   {
      std::cout << "failed to synchronize MPSSE with command 0xAA" << std::endl;
      return false;
   }

   std::cout << "MPSSE synchronized with command " << std::hex << (DWORD)command << std::endl;

   return true;
}

bool psvcd::SyncMMPSE(FT_HANDLE ftHandle)
{
   if(!EnableLoopback(ftHandle))
      return false;

   if(!SyncMPSSE(ftHandle, 0xAA))
      return false;

   if(!SyncMPSSE(ftHandle, 0xAB))
      return false;

   if(!DisableLoopback(ftHandle))
      return false;
  
   return true;
}

bool psvcd::ConfigureSettings(FT_HANDLE ftHandle)
{
   // Configure the MPSSE settings
   
   DWORD dwNumBytesToSend = 0;

   BYTE OutputBuffer[20];  //TODO: Implement as dynamic buffer

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x8A; // Use 60MHz master clock (disable divide by 5)
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x97; // Turn off adaptive clocking (may be needed for ARM)
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x8D; // Disable three-phase data clocking

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x9E; // Enable drive-zero mode on the lines used for I2C
                                                  // Used lines will be driven low when 0 and tristate when 1
                                                  // We in turn pull the lines with resistors to 3.3 power supply of regulator
                                            
                                                  // Which makes it possible to have 3.3v signals even 
                                                  // when regulator does not work on FT232H giving 3.8v instead of 3.3v
                                                  // though we currently use external regulator for 3.3 in self powered mode

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x27; // bits AD0 - clock, AD1 - dout, AD2 - din, AD5 - wait
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x7F; // bits AC0, AC1, AC2 - address (mux), AC3, AC4, AC5 - address (demux), AC6 - read/write, AC7 is not used because in self powered mode

   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x85; // disable internal loopback (so that data pins are not locked internally)

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); // Send off the commands

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

bool psvcd::WaitCycles(FT_HANDLE ftHandle, WORD nCycles)
{
   //wait 1 ms
   std::this_thread::sleep_for(std::chrono::milliseconds(1));

   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[20];  //TODO: Implement as dynamic buffer  

   // Clock bytes in
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x8F; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(nCycles & 0xFF);
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((nCycles >> 0x08) & 0xFF);

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB;

   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); //Send off the commands

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

bool psvcd::WaitPowerUp(FT_HANDLE ftHandle)
{
   //wait 1 ms
   std::this_thread::sleep_for(std::chrono::milliseconds(1));

   //then wait 64 cycles + 10 cycles = 74

   DWORD dwNumBytesToSend = 0;
   
   BYTE OutputBuffer[20];  //TODO: Implement as dynamic buffer

   // Clock bytes in 9*8 = 72
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x8F; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(0x08 & 0xFF); //Clock (8 + 1) = 9 bytes = 72 bits
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x00; 

   //clock bits in = 2
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x8E; // command: clock bytes out MSB first on clock falling edge
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x01; //Clock (1 + 1) = 2 bits

   //Enter idle state on ADbus
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x80; // Command to set ADbus direction/ data 
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xFE; // Set the value of the pins   
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0xDB;

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

bool psvcd::ConfigureDivisor(FT_HANDLE ftHandle, WORD dwClockDivisor)
{
   BYTE OutputBuffer[20];  //TODO: Implement as dynamic buffer

   DWORD dwNumBytesToSend = 0;

   // Command to set clock divisor
   OutputBuffer[dwNumBytesToSend++] = (BYTE)0x86;
   // Set 0xValueL of clock divisor
   OutputBuffer[dwNumBytesToSend++] = (BYTE)(dwClockDivisor & 0xFF);
   // Set 0xValueH of clock divisor
   OutputBuffer[dwNumBytesToSend++] = (BYTE)((dwClockDivisor >> 8) & 0xFF);
   
   DWORD dwNumBytesSent = 0;
   FT_STATUS ftStatus = FT_Write(ftHandle, OutputBuffer, dwNumBytesToSend, &dwNumBytesSent); // Send off the commands

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