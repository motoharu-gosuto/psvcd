cmake_minimum_required (VERSION 3.0)
project (dump_exfat)

message(dump_exfat)

set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../${PROJECT_NAME})

set(SOURCES ${SOURCE_PATH}/ExFatTypes.h
            ${SOURCE_PATH}/DumpExfat.h
            ${SOURCE_PATH}/DumpExfat.cpp
            ${SOURCE_PATH}/main.cpp
            )

add_executable(dump_exfat ${SOURCES})

set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)

set (BOOST_COMPONENTS system 
                      filesystem)

FIND_PACKAGE(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)

if(Boost_FOUND)
message("Using Boost_VERSION: ${Boost_VERSION}")
message("Using Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Using Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
else()
message("Boost library is not found")
endif()

target_link_libraries (dump_exfat ${Boost_LIBRARIES})

include_directories(..)
include_directories(${Boost_INCLUDE_DIRS})