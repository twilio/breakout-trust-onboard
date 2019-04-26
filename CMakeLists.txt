cmake_minimum_required(VERSION 3.7)
project(breakout-tob)

set(CMAKE_CXX_STANDARD 14)

option(PCSC_SUPPORT "Support fot pcsc-lite" OFF)
option(BUILD_SHARED "Build as a shared library" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(TOOL_BINARY trust_onboard_tool)
set(TOB_ENGINE trust_onboard_engine)

#set(CMAKE_BUILD_TYPE Debug)
#add_definitions(-DAT_DEBUG)
#add_definitions(-DMODEM_DEBUG)
#add_definitions(-DAPDU_DEBUG)
#add_definitions(-DSERIAL_DEBUG)

include_directories(external_libs/tob_sim/common/inc)
include_directories(external_libs/tob_sim/platform/generic_modem/inc)
include_directories(external_libs/tob_sim/platform/pcsc/inc)
include_directories(include)

set(LIB_SOURCES
        external_libs/tob_sim/common/src/Applet.cpp
        external_libs/tob_sim/common/src/MF.cpp
        external_libs/tob_sim/common/src/MIAS.cpp
        external_libs/tob_sim/common/src/NetInterface.cpp
        external_libs/tob_sim/common/src/SEInterface.cpp
        external_libs/tob_sim/common/src/base64.c
        external_libs/tob_sim/platform/generic_modem/src/ATInterface.cpp
        external_libs/tob_sim/platform/generic_modem/src/GenericModem.cpp
        external_libs/tob_sim/platform/generic_modem/src/LSerial.cpp
        external_libs/tob_sim/platform/generic_modem/src/Serial.cpp
        src/BreakoutTrustOnboardSDK.cpp
	)

set(LIB_HEADERS
        external_libs/tob_sim/common/inc/ISO7816.h
        external_libs/tob_sim/common/inc/Applet.h
        external_libs/tob_sim/common/inc/MF.h
        external_libs/tob_sim/common/inc/MIAS.h
        external_libs/tob_sim/common/inc/NetInterface.h
        external_libs/tob_sim/common/inc/SEInterface.h
        external_libs/tob_sim/common/inc/base64.h
        external_libs/tob_sim/platform/generic_modem/inc/ATInterface.h
        external_libs/tob_sim/platform/generic_modem/inc/GenericModem.h
        external_libs/tob_sim/platform/generic_modem/inc/LSerial.h
        external_libs/tob_sim/platform/generic_modem/inc/Serial.h
	)

if(PCSC_SUPPORT)
	find_package(PCSC REQUIRED)

        set(LIB_SOURCES ${LIB_SOURCES}
		external_libs/tob_sim/platform/pcsc/src/Pcsc.cpp)
	set(LIB_HEADERS ${LIB_HEADERS}
		external_libs/tob_sim/platform/pcsc/inc/Pcsc.h)
	include_directories(${PCSC_INCLUDE_DIRS})
	add_definitions(-DPCSC_SUPPORT)
endif(PCSC_SUPPORT)

if(BUILD_SHARED)
	add_library(TwilioTrustOnboard SHARED ${LIB_SOURCES} ${LIB_HEADERS})
else(BUILD_SHARED)
	add_library(TwilioTrustOnboard STATIC ${LIB_SOURCES} ${LIB_HEADERS})
endif(BUILD_SHARED)

set_property(TARGET TwilioTrustOnboard PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(
        ${TOOL_BINARY}
        tools/TrustOnboardTool.cpp)

add_library(
	${TOB_ENGINE} SHARED
	tools/OpenSSLEngine.cpp)

target_link_libraries(
        ${TOOL_BINARY}
        TwilioTrustOnboard
  )

target_link_libraries(
	${TOB_ENGINE}
        TwilioTrustOnboard
  )

if(PCSC_SUPPORT)
	target_link_libraries(
		${TOOL_BINARY}
                ${PCSC_LIBRARIES}
        )

	target_link_libraries(
		${TOB_ENGINE}
                ${PCSC_LIBRARIES}
        )
endif(PCSC_SUPPORT)

set(TOOLS_SOURCES
        tools/TrustOnboardTool.cpp
	tools/OpenSSLEngine.cpp
	)

add_custom_target(format COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/service-scripts/format.sh ${LIB_SOURCES} ${LIB_HEADERS} ${TOOLS_SOURCES} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS TwilioTrustOnboard DESTINATION lib)
install(TARGETS ${TOOL_BINARY} DESTINATION bin)
# TODO: set group to dialout?
install(FILES include/BreakoutTrustOnboardSDK.h DESTINATION include)

set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

set(CPACK_PACKAGE_NAME "trust-onboard-sdk")
set(CPACK_PACKAGE_VENDOR "Twilio Inc.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SDK and tooling for Trust Onboard")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_SYSTEM_NAME "Linux")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Twilio Inc.")
set(CPACK_SET_DESTDIR "ON")

if(TOB_DEVICE)
	add_subdirectory(device-support/${TOB_DEVICE})
endif(TOB_DEVICE)

include(CPack)

add_subdirectory(tests)
