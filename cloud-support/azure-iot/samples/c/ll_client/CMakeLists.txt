#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for remote monitoring sample

cmake_minimum_required(VERSION 3.5)

set(ll_client_c_files
    ll_client.c
    ../../../azure-iot-sdk-c/custom_hsm_twilio.cpp
)

include_directories(.)
include_directories(../../../azure-iot-sdk-c)
include_directories(/usr/local/include/azureiot)

add_executable(ll_client ${ll_client_c_files})

target_link_libraries(ll_client
    TwilioTrustOnboard
    prov_http_transport
    iothub_client
    prov_device_ll_client
    prov_auth_client
    hsm_security_client
    uhttp
    umqtt
    utpm
    msr_riot
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
