cmake_minimum_required(VERSION 3.7)
project(breakout-tob)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(external_libs/gemalto/common/inc)
include_directories(external_libs/gemalto/mbedtls/inc)
include_directories(external_libs/gemalto/platform/generic_module/inc)
include_directories(include)

add_executable(trust_onboard_tool
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
        external_libs/gemalto/platform/generic_module/inc/ATInterface.h
        external_libs/gemalto/platform/generic_module/inc/GenericModem.h
        external_libs/gemalto/platform/generic_module/inc/LSerial.h
        external_libs/gemalto/platform/generic_module/inc/Serial.h
        external_libs/gemalto/platform/generic_module/src/ATInterface.cpp
        external_libs/gemalto/platform/generic_module/src/GenericModem.cpp
        external_libs/gemalto/platform/generic_module/src/LSerial.cpp
        external_libs/gemalto/platform/generic_module/src/Serial.cpp
        include/BreakoutTrustOnboardSDK.h
        src/BreakoutTrustOnboardSDK.cpp
        tools/TrustOnboardTool.cpp
        tools/README.md)
