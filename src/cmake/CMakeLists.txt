cmake_minimum_required (VERSION 3.0)

SET(EXECUTABLES dump_exfat mmc_card sd_card)
SET(LIBRARIES common)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

include(${CMAKE_SOURCE_DIR}/scripts/ConfigureDependencies.cmake)

configure_ftdi()

add_definitions(-DMMC_PINOUT)

foreach(library ${LIBRARIES})
   include(${CMAKE_SOURCE_DIR}/../${library}/cmake/CMakeLists.txt)
endforeach(library)

foreach(executable ${EXECUTABLES})
   include(${CMAKE_SOURCE_DIR}/../${executable}/cmake/CMakeLists.txt)
endforeach(executable)

project (psvcd)