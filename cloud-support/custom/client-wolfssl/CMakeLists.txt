cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake_modules/)

if (LINUX)
  set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

find_package(WolfSSL REQUIRED)
include_directories(${WOLFSSL_INCLUDE_DIRS})


add_executable(client_sample client_sample.cpp)

target_link_libraries(client_sample
  ${WOLFSSL_LIBRARIES}
  TwilioTrustOnboard
)
