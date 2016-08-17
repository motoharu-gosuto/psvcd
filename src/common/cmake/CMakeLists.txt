cmake_minimum_required (VERSION 3.0)
project (common)

message(common)

set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME})

set(SOURCES ${SOURCE_PATH}/CardRoutines.h
            ${SOURCE_PATH}/CardRoutines.cpp
            ${SOURCE_PATH}/CRC.h
            ${SOURCE_PATH}/CRC.cpp
            ${SOURCE_PATH}/FTDICommon.h
            ${SOURCE_PATH}/FTDICommon.cpp
            ${SOURCE_PATH}/FTDIInitialization.h
            ${SOURCE_PATH}/FTDIInitialization.cpp
            ${SOURCE_PATH}/GenericPins.h
            ${SOURCE_PATH}/MMCPins.h
            ${SOURCE_PATH}/SDPins.h
            )

add_library(common ${SOURCES})

target_link_libraries(common ${FTDI_LIBRARIES})

include_directories(..)
include_directories(${FTDI_INCLUDEDIR})