cmake_minimum_required(VERSION 3.7)
project(breakout-tob-tests)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(
	trust_onboard_sdk_tests
	BreakoutTrustOnboardSDKTests.cpp
)

add_executable(
	trust_onboard_ll_tests
	BreakoutTrustOnboardLLTests.cpp
)

target_link_libraries(
	trust_onboard_sdk_tests
	TwilioTrustOnboard
)

target_link_libraries(
	trust_onboard_ll_tests
	TwilioTrustOnboard
	${OPENSSL_LIBRARIES}
)


if(PCSC_SUPPORT)
	target_link_libraries(
		trust_onboard_sdk_tests
		${PCSC_LIBRARIES}
	)

	target_link_libraries(
		trust_onboard_ll_tests
		${PCSC_LIBRARIES}
	)
endif(PCSC_SUPPORT)

