cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake_modules/)

if (LINUX)
  set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

find_package(MbedTLS REQUIRED)
include_directories(${MBEDTLS_INCLUDE_DIRS})

if (TRUST_ONBOARD_INCLUDE_DIRS)
  include_directories(${TRUST_ONBOARD_INCLUDE_DIRS})
endif()

find_package(Threads REQUIRED)

find_library(TRUST_ONBOARD NAMES TwilioTrustOnboard)
find_library(TRUST_ONBOARD_MBED NAMES tob_mbed)

add_executable(client_sample client_sample.cpp)

target_link_libraries(client_sample
  ${CMAKE_THREAD_LIBS_INIT}
  ${MBEDTLS_LIBRARIES}
  ${TRUST_ONBOARD}
  ${TRUST_ONBOARD_MBED}
)
