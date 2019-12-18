cmake_minimum_required(VERSION 3.7)
project(breakout-tob)

set(CMAKE_CXX_STANDARD 14)
option(PCSC_SUPPORT "Support for pcsc-lite" OFF)
option(OPENSSL_SUPPORT "Support for signing key with OpenSSL support" OFF)
option(MBEDTLS_SUPPORT "Private crypto device shims for MbedTLS 2.11+" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DOCS "Build doxygen documentation" OFF)
option(BUILD_AZURE "Build with Azure IoT Hub support" OFF)
option(NO_OS "Build for bare metal / RTOS" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/)

if(ENABLE_COVERAGE)
	find_package(codecov)
endif(ENABLE_COVERAGE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(TOOL_BINARY trust_onboard_tool)
if(OPENSSL_SUPPORT)
	set(TOB_ENGINE trust_onboard_engine)
	add_definitions(-DOPENSSL_SUPPORT)
endif(OPENSSL_SUPPORT)

if(BUILD_AZURE)
	set(TOB_DPS_REGISTERER azure_dps_registerer)
endif(BUILD_AZURE)

if(NO_OS)
	add_definitions(-DNO_OS)

	if(PCSC_SUPPORT)
		message(FATAL_ERROR "PC/SC support requires an OS environment")
	endif(PCSC_SUPPORT)

	if(OPENSSL_SUPPORT)
		message(FATAL_ERROR "OpenSSL support requires an OS environment")
	endif(OPENSSL_SUPPORT)

	if(BUILD_AZURE)
		message(FATAL_ERROR "Azure IoT support requires an OS environment")
	endif(BUILD_AZURE)

	if(BUILD_TESTS)
		message(FATAL_ERROR "Testing requires an OS environment")
	endif(BUILD_TESTS)
endif(NO_OS)

include_directories(external_libs/tob_sim/common/inc)
include_directories(external_libs/tob_sim/platform/generic_modem/inc)
include_directories(external_libs/tob_sim/platform/pcsc/inc)
include_directories(include)

set(LIB_SOURCES
        external_libs/tob_sim/common/src/Applet.cpp
        external_libs/tob_sim/common/src/MF.cpp
        external_libs/tob_sim/common/src/MIAS.cpp
        external_libs/tob_sim/common/src/SEInterface.cpp
        external_libs/tob_sim/common/src/base64.c
        src/BreakoutTrustOnboardSDK.cpp
	)

set(LIB_HEADERS
        external_libs/tob_sim/common/inc/ISO7816.h
        external_libs/tob_sim/common/inc/Applet.h
        external_libs/tob_sim/common/inc/MF.h
        external_libs/tob_sim/common/inc/MIAS.h
        external_libs/tob_sim/common/inc/SEInterface.h
        external_libs/tob_sim/common/inc/base64.h
	)

if(NOT NO_OS)
	set(LIB_SOURCES ${LIB_SOURCES}
		external_libs/tob_sim/platform/generic_modem/src/ATInterface.cpp
		external_libs/tob_sim/platform/generic_modem/src/GenericModem.cpp
		external_libs/tob_sim/platform/generic_modem/src/LSerial.cpp
		external_libs/tob_sim/platform/generic_modem/src/Serial.cpp)

	set(LIB_HEADERS ${LIB_HEADERS}
		external_libs/tob_sim/platform/generic_modem/inc/ATInterface.h
		external_libs/tob_sim/platform/generic_modem/inc/GenericModem.h
		external_libs/tob_sim/platform/generic_modem/inc/LSerial.h
		external_libs/tob_sim/platform/generic_modem/inc/Serial.h)
endif(NOT NO_OS)

if(PCSC_SUPPORT)
	find_package(PCSC REQUIRED)

        set(LIB_SOURCES ${LIB_SOURCES}
		external_libs/tob_sim/platform/pcsc/src/Pcsc.cpp)
	set(LIB_HEADERS ${LIB_HEADERS}
		external_libs/tob_sim/platform/pcsc/inc/Pcsc.h)
	include_directories(${PCSC_INCLUDE_DIRS})
	add_definitions(-DPCSC_SUPPORT)
endif(PCSC_SUPPORT)

if(BUILD_AZURE)
	set(LIB_SOURCES ${LIB_SOURCES} src/TobAzureHsm.cpp)
	set(LIB_HEADERS ${LIB_HEADERS} include/TobAzureHsm.h)
endif(BUILD_AZURE)

add_library(TwilioTrustOnboard SHARED ${LIB_SOURCES} ${LIB_HEADERS})
add_coverage(TwilioTrustOnboard)

set_property(TARGET TwilioTrustOnboard PROPERTY POSITION_INDEPENDENT_CODE ON)

if(NOT NO_OS)
	find_package(NlohmannJson REQUIRED)
	add_executable(
        	${TOOL_BINARY}
	        tools/TrustOnboardTool.cpp)
	add_coverage(${TOOL_BINARY})
	target_include_directories(${TOOL_BINARY}
		PRIVATE
		${JSON_INCLUDE_DIR})

	target_link_libraries(
        	${TOOL_BINARY}
        	TwilioTrustOnboard)
endif(NOT NO_OS)

if(OPENSSL_SUPPORT)
	find_package(OpenSSL REQUIRED)
	add_library(
		${TOB_ENGINE} SHARED
		tools/OpenSSLEngine.cpp)
	add_coverage(${TOB_ENGINE})

	target_link_libraries(
		${TOB_ENGINE}
		TwilioTrustOnboard
		${OPENSSL_LIBRARIES})

	target_include_directories(
		${TOB_ENGINE} PRIVATE
		${OPENSSL_INCLUDE_DIR})

	target_link_libraries(
		${TOOL_BINARY}
		${OPENSSL_LIBRARIES})

	target_include_directories(
		${TOOL_BINARY} PRIVATE
		${OPENSSL_INCLUDE_DIR})
endif(OPENSSL_SUPPORT)

if(MBEDTLS_SUPPORT)
	find_package(MbedTLS REQUIRED)
	add_library(
		tob_mbed SHARED
		src/TobMbedtls.cpp)
	add_coverage(${tob_mbed})
	target_include_directories(tob_mbed PUBLIC ${MBEDTLS_INCLUDE_DIRS})
	target_link_libraries(tob_mbed ${MBEDTLS_LIBRARIES} TwilioTrustOnboard)
endif(MBEDTLS_SUPPORT)

if(BUILD_AZURE)
	find_package(OpenSSL REQUIRED)
	find_package(Threads REQUIRED)

	add_executable(${TOB_DPS_REGISTERER}
		tools/AzureDpsRegisterer.cpp)
	add_coverage(${TOB_DPS_REGISTERER})
	target_link_libraries(
		${TOB_DPS_REGISTERER}
		TwilioTrustOnboard
		prov_http_transport # This and below are static Azure IoT SDK libraries
		iothub_client
		prov_device_ll_client
		prov_auth_client
		hsm_security_client
		uhttp
		iothub_client_http_transport
		aziotsharedutil
		parson # End of Azure libraries
		m # Libm is required by Azure libraries, so goes after Azure libraries
		${CMAKE_THREAD_LIBS_INIT} # So is pthread
		)
	target_include_directories(${TOB_DPS_REGISTERER} PRIVATE ${JSON_INCLUDE_DIR})

	# OpenSSL is required for Azure custom HSM
	target_include_directories(TwilioTrustOnboard PUBLIC
		${OPENSSL_INCLUDE_DIR})
	target_link_libraries(
		TwilioTrustOnboard
		${OPENSSL_LIBRARIES})
endif(BUILD_AZURE)

if(PCSC_SUPPORT)
	target_link_libraries(
		TwilioTrustOnboard
		${PCSC_LIBRARIES}
	)
endif(PCSC_SUPPORT)

set(TOOLS_SOURCES
        tools/TrustOnboardTool.cpp
        tools/OpenSSLEngine.cpp
        tools/AzureDpsRegisterer.cpp
	)

add_custom_target(format COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/service-scripts/format.sh ${LIB_SOURCES} ${LIB_HEADERS} ${TOOLS_SOURCES} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_DOCS)
	add_custom_target(docs ALL COMMAND doxygen ${CMAKE_CURRENT_SOURCE_DIR}/service-scripts/doxygen.conf)
endif (BUILD_DOCS)

install(TARGETS TwilioTrustOnboard DESTINATION lib)
if(OPENSSL_SUPPORT)
	install(TARGETS ${TOB_ENGINE} DESTINATION lib)
endif(OPENSSL_SUPPORT)
if(BUILD_AZURE)
	install(TARGETS ${TOB_DPS_REGISTERER} DESTINATION bin)
	install(FILES include/TobAzureHsm.h DESTINATION include)
endif(BUILD_AZURE)
if(MBEDTLS_SUPPORT)
	install(TARGETS tob_mbed DESTINATION lib)
	install(FILES include/TobMbedtls.h DESTINATION include)
endif(MBEDTLS_SUPPORT)

if(NOT NO_OS)
	install(TARGETS ${TOOL_BINARY} DESTINATION bin)
endif(NOT NO_OS)

install(FILES include/BreakoutTrustOnboardSDK.h
	      external_libs/tob_sim/common/inc/SEInterface.h
	      external_libs/tob_sim/common/inc/ISO7816.h
	DESTINATION include)

file(GLOB_RECURSE CA_CERTS "${PROJECT_SOURCE_DIR}/bundles/*.pem" "${PROJECT_SOURCE_DIR}/bundles/*.0")
install(FILES ${CA_CERTS} DESTINATION share/trust_onboard/ssl/ca)
install(FILES tls-conf/acm.cnf tls-conf/pcsc.cnf DESTINATION share/trust_onboard/ssl/)
install(DIRECTORY samples/azure-iot DESTINATION share/trust_onboard/samples USE_SOURCE_PERMISSIONS)
install(DIRECTORY samples/standalone DESTINATION share/trust_onboard/samples USE_SOURCE_PERMISSIONS)

function (depends_append DEPLIST_TO_APPEND FIRST_DEP)
	set(DEPLIST_LOCAL "${${DEPLIST_TO_APPEND}}")
	if("${DEPLIST_LOCAL}" STREQUAL "")
		set(DEPLIST_LOCAL "${FIRST_DEP}")
	else()
		set(DEPLIST_LOCAL "${DEPLIST_LOCAL}, ${FIRST_DEP}")
	endif()

	foreach(dep ${ARGN})
		set(DEPLIST_LOCAL "${DEPLIST_LOCAL}, ${dep}")
	endforeach(dep)

	set(${DEPLIST_TO_APPEND} "${DEPLIST_LOCAL}" PARENT_SCOPE)
endfunction()

set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

set(CPACK_PACKAGE_NAME "trust-onboard-sdk")
set(CPACK_PACKAGE_VENDOR "Twilio Inc.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SDK and tooling for Trust Onboard")
SET(CPACK_SYSTEM_NAME "Linux")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Twilio Inc.")
if(OPENSSL_SUPPORT OR BUILD_AZURE)
	if(OPENSSL_VERSION_MINOR EQUAL "1")
		depends_append(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl-dev")
	elseif(OPENSSL_VERSION_MINOR EQUAL "0")
		depends_append(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl1.0-dev")
	else()
		message(FATAL_ERROR "Unsupported OpenSSL version: ${OPENSSL_VERSION}")
	endif()
endif()

if(PCSC_SUPPORT)
	depends_append(CPACK_DEBIAN_PACKAGE_DEPENDS "libpcsclite1")
endif()

if(BUILD_AZURE)
	depends_append(CPACK_DEBIAN_PACKAGE_DEPENDS "azure-iot-sdk-c-twilio-dev")
endif()

set(CPACK_SET_DESTDIR "ON")

if(TOB_DEVICE)
	add_subdirectory(device-support/${TOB_DEVICE})
endif(TOB_DEVICE)

include(CPack)

if (BUILD_TESTS)
	add_subdirectory(tests)
endif (BUILD_TESTS)
