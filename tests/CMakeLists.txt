cmake_minimum_required(VERSION 3.7)
project(breakout-tob-tests)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(TEST_BINARY tests)

add_executable(
  ${TEST_BINARY}
  tests.cpp
  BreakoutTrustOnboardSDKTests.cpp
  )

target_link_libraries(
  ${TEST_BINARY}
  TwilioTrustOnboard
  )

if(PCSC_SUPPORT)
  target_link_libraries(
    ${TEST_BINARY}
    ${PCSC_LIBRARIES}
  )
endif(PCSC_SUPPORT)

