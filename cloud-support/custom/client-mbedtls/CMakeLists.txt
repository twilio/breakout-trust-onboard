cmake_minimum_required(VERSION 3.7)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake_modules/)

if (LINUX)
  set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

find_package(MbedTLS REQUIRED)
include_directories(${MBEDTLS_INCLUDE_DIRS})

find_package(Threads REQUIRED)

add_executable(client_sample client_sample.cpp)

target_link_libraries(client_sample
  tob_mbed
  ${CMAKE_THREAD_LIBS_INIT}
  ${MBEDTLS_LIBRARIES}
)
