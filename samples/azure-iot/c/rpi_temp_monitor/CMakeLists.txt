#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for remote monitoring sample

cmake_minimum_required(VERSION 3.5)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

if (NOT AZURE_ID_SCOPE_PATH)
  set(AZURE_ID_SCOPE_PATH "/home/pi/azure_id_scope.txt")
endif()
add_definitions(-DAZURE_ID_SCOPE_PATH="${AZURE_ID_SCOPE_PATH}")

add_executable(rpi_temp_monitor rpi_temp_monitor.c)

target_link_libraries(rpi_temp_monitor
    TwilioTrustOnboard
    prov_http_transport
    iothub_client
    prov_device_ll_client
    prov_auth_client
    hsm_security_client
    uhttp
    umqtt
    iothub_client_http_transport
    iothub_client_mqtt_transport
    prov_auth_client
    aziotsharedutil
    curl
    ssl
    crypto
    pthread
    m
    parson)

add_definitions(-DUSE_MQTT)
