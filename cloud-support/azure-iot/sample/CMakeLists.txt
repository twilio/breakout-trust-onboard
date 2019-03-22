#this is CMakeLists.txt for hello_register 

cmake_minimum_required(VERSION 3.7)

include("helpers.cmake")

setOSFlags()

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

compileAsC99()

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
  /usr/local/include
  /usr/local/include/azureiot
  /usr/local/opt/openssl/include
  ../azure-iot-sdk-c
  )

link_directories(
  /usr/local/lib
  /usr/local/opt/openssl/lib
  )

set(libraries_to_link
    TwilioTrustOnboard
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
    curl
    ssl
    crypto
    pthread
    m
    parson
)

add_executable(hello_register ${hello_register_c_files} ${hello_register_h_files})

target_link_libraries(hello_register
  ${libraries_to_link}
)

add_definitions(-DUSE_OPENSSL)
