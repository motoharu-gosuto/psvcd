#pragma once

//TODO: need to merge server and client definitions into single file
//these are much better than original on server

#define PSVKIRK_COMMAND_PING 0
#define PSVKIRK_COMMAND_TERM 1
#define PSVKIRK_COMMAND_GEN10 2
#define PSVKIRK_COMMAND_GEN20 3
#define PSVKIRK_COMMAND_KIRK 4

#pragma pack(push, 1)

struct command_response_base
{
   uint32_t command;
   uint32_t vita_err;
   uint32_t proxy_err;
};

//connection test command
struct command_0_request
{
   uint32_t command;
};

struct command_0_response
{
   command_response_base base;
   uint8_t data[10];
};

//connection terminate command
struct command_1_request
{
   uint32_t command;
};

struct command_1_response
{
   command_response_base base;
   uint8_t data[10];
};

//gen 0x10 command
struct command_2_request
{
   uint32_t command;
};

struct command_2_response
{
   command_response_base base;
   uint8_t data[0x10];
};

//gen 0x20 command
struct command_3_request
{
   uint32_t command;
};

struct command_3_response
{
   command_response_base base;
   uint8_t data[0x20];
};

//call kirk service command
struct command_4_request
{
   uint32_t command;
   uint32_t kirk_command;
   uint32_t size;
   uint32_t kirk_param;
   uint8_t data[0x800];
};

struct command_4_response
{
   command_response_base base;
   uint32_t size;
   uint8_t data[0x800];
};

//--------------------------------

struct kirk_1B_input
{
   uint8_t packet6[0x20];
   uint8_t packet7[0x10];
   uint8_t packet8[0x23];
};

struct kirk_1C_input
{
   uint8_t packet6[0x20];
   uint8_t packet8[0x20];
};

#pragma pack(pop)

int initialize_kirk_proxy_connection(SOCKET& ConnectSocket);

int deinitialize_kirk_proxy_connection(SOCKET ConnectSocket);