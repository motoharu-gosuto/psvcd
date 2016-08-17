cmake_minimum_required (VERSION 3.0)
project (mmc_card)

message(mmc_card)

set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME})

set(SOURCES ${SOURCE_PATH}/CMD56Initialization.h
            ${SOURCE_PATH}/CMD56Initialization.cpp
            ${SOURCE_PATH}/CMD56Packets.h
            ${SOURCE_PATH}/CMD56Packets.cpp
            ${SOURCE_PATH}/main.cpp
            ${SOURCE_PATH}/MMCCommands.h
            ${SOURCE_PATH}/MMCCommands.cpp
            ${SOURCE_PATH}/MMCDump.h
            ${SOURCE_PATH}/MMCDump.cpp
            ${SOURCE_PATH}/MMCInitialization.h
            ${SOURCE_PATH}/MMCInitialization.cpp
            ${SOURCE_PATH}/MMCTypes.h
            ${SOURCE_PATH}/MMCTypes.cpp
            )

add_executable(mmc_card ${SOURCES})

target_link_libraries (mmc_card common)

include_directories(..)