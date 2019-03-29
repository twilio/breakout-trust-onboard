#this is CMakeLists.txt for hello_register 

cmake_minimum_required(VERSION 3.7)

# To override MODULE_DEVICE or SIM_PIN:
# cmake -DMODULE_DEVICE="/dev/ttyACM1" -DSIM_PIN="0000" ..

if (NOT MODULE_DEVICE)
  set(MODULE_DEVICE "/dev/ttyACM1")
endif()

if (NOT SIM_PIN)
  set(SIM_PIN "0000")
endif()

add_definitions(-DMODULE_DEVICE="${MODULE_DEVICE}")
add_definitions(-DSIM_PIN="${SIM_PIN}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (LINUX)
  set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(hello_register_c_files
    hello_register.c
    ../azure-iot-sdk-c/base64.c
    ../azure-iot-sdk-c/custom_hsm_twilio.cpp
)

set (hello_register_h_files
    ../azure-iot-sdk-c/base64.h
    ../azure-iot-sdk-c/custom_hsm_twilio.h
)

include_directories(
	../azure-iot-sdk-c
	)


find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

find_package(Threads REQUIRED)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

set(AZURE_SDK_LIBRARIES
    prov_http_transport
    iothub_client
    prov_device_client
    prov_device_ll_client
    prov_auth_client
    hsm_security_client
    uhttp
    iothub_client_http_transport
    iothub_client_mqtt_transport
    iothub_client_mqtt_ws_transport
    prov_auth_client
    umqtt
    aziotsharedutil
    parson
    )

add_executable(hello_register ${hello_register_c_files} ${hello_register_h_files})

target_link_libraries(hello_register
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENSSL_LIBRARIES}
  ${CURL_LIBRARIES}
  ${AZURE_SDK_LIBRARIES}
  TwilioTrustOnboard
)

install(TARGETS hello_register DESTINATION bin)

add_definitions(-DUSE_OPENSSL)

set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

set(CPACK_PACKAGE_NAME "Azure DPS registation sample")
set(CPACK_PACKAGE_VENDOR "Twilio Inc.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Sample utility performing X509 DPS registraion")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_SYSTEM_NAME "Linux")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Twilio Inc.")
set(CPACK_SET_DESTDIR "ON")

include(CPack)


