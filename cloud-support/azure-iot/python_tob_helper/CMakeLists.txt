cmake_minimum_required(VERSION 3.7)

# cmake -DMODULE_DEVICE="/dev/cu.usbmodem14103" -DSIM_PIN="0000" ..
# cmake -DMODULE_DEVICE="/dev/ttyACM1" -DSIM_PIN="0000" ..

option (USE_SIGNING "Use signing key and certificate" ON)

if (NOT MODULE_DEVICE)
  set(MODULE_DEVICE "/dev/ttyACM1")
endif()

if (NOT SIM_PIN)
  set(SIM_PIN "0000")
endif()

if (NOT AZURE_ID_SCOPE_PATH)
  set(AZURE_ID_SCOPE_PATH "/home/pi/azure_id_scope.txt")
endif()

add_definitions(-DMODULE_DEVICE="${MODULE_DEVICE}")
add_definitions(-DSIM_PIN="${SIM_PIN}")
add_definitions(-DAZURE_ID_SCOPE_PATH="${AZURE_ID_SCOPE_PATH}")

if (USE_SIGNING)
  add_definitions(-DUSE_SIGNING)
endif (USE_SIGNING)

set(TOOL_BINARY python_tob_helper)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX TRUE)
  # on Linux, enable valgrind
  # these commands (MEMORYCHECK...) need to apear BEFORE include(CTest) or they will not have any effect
  find_program(MEMORYCHECK_COMMAND valgrind)
  set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1")
  set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(MACOSX TRUE)
  add_definitions(-DMACOSX)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (LINUX)
  set (CMAKE_CXX_FLAGS "-pthread ${CMAKE_CXX_FLAGS}")
endif()

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

set(python_tob_helper_c_files
    python_tob_helper.c
    ../azure-iot-sdk-c/custom_hsm_twilio.cpp
)

set (python_tob_helper_h_files
)

include_directories(
  /usr/local/include
  /usr/local/include/azureiot
  /usr/local/opt/openssl/include
  ../azure-iot-sdk-c/
  )

link_directories(
  /usr/local/lib
  /usr/local/opt/openssl/lib
  )

set(libraries_to_link
    TwilioTrustOnboard
    prov_http_transport
    iothub_client
    prov_device_ll_client
    prov_auth_client
    hsm_security_client
    uhttp
    iothub_client_http_transport
    prov_auth_client
    aziotsharedutil
    curl
    ssl
    crypto
    pthread
    m
    parson
)

add_executable(${TOOL_BINARY} ${python_tob_helper_c_files} ${python_tob_helper_h_files})

target_link_libraries(${TOOL_BINARY}
  ${libraries_to_link}
)

add_definitions(-DUSE_OPENSSL)

install(TARGETS ${TOOL_BINARY} DESTINATION bin)
