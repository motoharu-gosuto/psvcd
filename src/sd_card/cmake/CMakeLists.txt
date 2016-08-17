cmake_minimum_required (VERSION 3.0)
project (sd_card)

message(sd_card)

set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME})

set(SOURCES ${SOURCE_PATH}/main.cpp
            ${SOURCE_PATH}/SDCommands.h
            ${SOURCE_PATH}/SDCommands.cpp
            ${SOURCE_PATH}/SDInitialization.h
            ${SOURCE_PATH}/SDInitialization.cpp
            ${SOURCE_PATH}/SDTypes.h
            ${SOURCE_PATH}/SDTypes.cpp
            )

add_executable(sd_card ${SOURCES})

target_link_libraries (sd_card common)

include_directories(..)