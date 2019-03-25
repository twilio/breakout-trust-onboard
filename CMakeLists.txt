cmake_minimum_required(VERSION 3.7)
project(breakout-tob)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(TOOL_BINARY trust_onboard_tool)

#add_definitions(-DAT_DEBUG)
#add_definitions(-DMODEM_DEBUG)
#add_definitions(-DAPDU_DEBUG)
#add_definitions(-DSERIAL_DEBUG)

include_directories(external_libs/tob_sim/common/inc)
include_directories(external_libs/tob_sim/platform/generic_modem/inc)
include_directories(include)

set(LIB_SOURCES
        external_libs/tob_sim/common/src/Applet.cpp
        external_libs/tob_sim/common/src/MF.cpp
        external_libs/tob_sim/common/src/MIAS.cpp
        external_libs/tob_sim/common/src/NetInterface.cpp
        external_libs/tob_sim/common/src/SEInterface.cpp
        external_libs/tob_sim/platform/generic_modem/src/ATInterface.cpp
        external_libs/tob_sim/platform/generic_modem/src/GenericModem.cpp
        external_libs/tob_sim/platform/generic_modem/src/LSerial.cpp
        external_libs/tob_sim/platform/generic_modem/src/Serial.cpp
        src/BreakoutTrustOnboardSDK.cpp
	)

set(LIB_HEADERS
        external_libs/tob_sim/common/inc/Applet.h
        external_libs/tob_sim/common/inc/MF.h
        external_libs/tob_sim/common/inc/MIAS.h
        external_libs/tob_sim/common/inc/NetInterface.h
        external_libs/tob_sim/common/inc/SEInterface.h
        external_libs/tob_sim/platform/generic_modem/inc/ATInterface.h
        external_libs/tob_sim/platform/generic_modem/inc/GenericModem.h
        external_libs/tob_sim/platform/generic_modem/inc/LSerial.h
        external_libs/tob_sim/platform/generic_modem/inc/Serial.h
	)

add_library(TwilioTrustOnboard STATIC ${LIB_SOURCES} ${LIB_HEADERS})

add_executable(
        ${TOOL_BINARY}
        tools/TrustOnboardTool.cpp)

target_link_libraries(
        ${TOOL_BINARY}
        TwilioTrustOnboard
  )

set(TOOLS_SOURCES
        tools/TrustOnboardTool.cpp)

add_custom_target(format COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/service-scripts/format.sh ${LIB_SOURCES} ${LIB_HEADERS} ${TOOLS_SOURCES} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS TwilioTrustOnboard DESTINATION lib)
install(TARGETS ${TOOL_BINARY} DESTINATION bin)
# TODO: set group to dialout?
install(FILES include/BreakoutTrustOnboardSDK.h DESTINATION include)

add_subdirectory(tests)
