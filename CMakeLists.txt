cmake_minimum_required(VERSION 3.7)
project(breakout-tob)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(external_libs/gemalto/common/inc)
include_directories(external_libs/gemalto/mbedtls/inc)
include_directories(external_libs/gemalto/platform/concept_board/inc)
include_directories(include)

add_executable(trust_on_board_tool
        external_libs/gemalto/common/inc/Applet.h
        external_libs/gemalto/common/inc/MF.h
        external_libs/gemalto/common/inc/MIAS.h
        external_libs/gemalto/common/inc/NetInterface.h
        external_libs/gemalto/common/inc/SEInterface.h
        external_libs/gemalto/common/src/Applet.cpp
        external_libs/gemalto/common/src/MF.cpp
        external_libs/gemalto/common/src/MIAS.cpp
        external_libs/gemalto/common/src/NetInterface.cpp
        external_libs/gemalto/common/src/SEInterface.cpp
        external_libs/gemalto/platform/concept_board/inc/ATInterface.h
        external_libs/gemalto/platform/concept_board/inc/CinterionModem.h
        external_libs/gemalto/platform/concept_board/inc/LSerial.h
        external_libs/gemalto/platform/concept_board/inc/Serial.h
        external_libs/gemalto/platform/concept_board/src/ATInterface.cpp
        external_libs/gemalto/platform/concept_board/src/CinterionModem.cpp
        external_libs/gemalto/platform/concept_board/src/LSerial.cpp
        external_libs/gemalto/platform/concept_board/src/Serial.cpp
        include/breakout_tob.h
        src/breakout_tob.cpp
        tools/trust_on_board_tool.cpp
        tools/README.md)
