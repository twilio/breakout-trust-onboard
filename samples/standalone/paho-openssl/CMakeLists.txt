cmake_minimum_required(VERSION 3.7)

if (LINUX)
	set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_BUILD_TYPE Debug)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(Threads REQUIRED)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

add_subdirectory(./libs/paho.mqtt.embedded-c)
include_directories(./libs/paho.mqtt.embedded-c/MQTTClient/src)
include_directories(./libs/paho.mqtt.embedded-c/MQTTPacket/src)

include_directories(./linuxToB)

add_executable(client_sample client_sample.cpp)

target_link_libraries(client_sample
	${CMAKE_THREAD_LIBS_INIT}
	${OPENSSL_LIBRARIES}
	${CURL_LIBRARIES}
	MQTTPacketClient
)
target_compile_definitions(client_sample PRIVATE MQTTCLIENT_PLATFORM_HEADER=MQTTLinuxToB.h)
