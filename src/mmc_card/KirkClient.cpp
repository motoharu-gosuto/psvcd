#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdint.h>

#include "KirkClient.h"

//TODO: these libraries should be moved to cmake lists

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_PORT "1330"
#define NODE_NAME "192.168.0.34"

int initialize_kirk_proxy_connection(SOCKET& ConnectSocket)
{
   std::cout << "Welcome to Kirk client" << std::endl;

   // Initialize Winsock
   WSADATA wsaData;

   int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
   if(iResult != 0) 
   {
      std::cout << "WSAStartup failed with error: " << iResult << std::endl;
      return -1;
   }

   ADDRINFOA hints;
   ZeroMemory(&hints, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;

   ADDRINFOA* result = NULL;

   // Resolve the server address and port
   iResult = getaddrinfo(NODE_NAME, DEFAULT_PORT, &hints, &result);
   if( iResult != 0 ) 
   {
      std::cout << "getaddrinfo failed with error:" << iResult << std::endl;
      WSACleanup();
      return -1;
   }

   // Attempt to connect to an address until one succeeds
   ADDRINFOA* ptr = NULL;
   ConnectSocket = INVALID_SOCKET;

   for(ptr = result; ptr != NULL ; ptr = ptr->ai_next) 
   {
      // Create a SOCKET for connecting to server
      ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
      if (ConnectSocket == INVALID_SOCKET) 
      {
         std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
         WSACleanup();
         return -1;
      }

      // Connect to server.
      iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
      if (iResult == SOCKET_ERROR) 
      {
         closesocket(ConnectSocket);
         ConnectSocket = INVALID_SOCKET;
         continue;
      }

      break;
   }

   freeaddrinfo(result);

   if(ConnectSocket == INVALID_SOCKET) 
   {
      std::cout << "Unable to connect to server!" << std::endl;
      WSACleanup();
      return -1;
   }

   return 0;
}

int deinitialize_kirk_proxy_connection(SOCKET ConnectSocket)
{
   // shutdown the connection since no more data will be sent
   int iResult = shutdown(ConnectSocket, SD_SEND);
   if (iResult == SOCKET_ERROR) 
   {
      std::cout << "shutdown failed with error: " << WSAGetLastError() << std::endl;
      closesocket(ConnectSocket);
      WSACleanup();
      return -1;
   }

   // cleanup
   closesocket(ConnectSocket);
   WSACleanup();

   return 0;
}