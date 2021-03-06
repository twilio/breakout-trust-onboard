// Copyright (c) Microsoft. All rights reserved.
// Copyright (c) Twilio. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "azureiot/iothub.h"
#include "azureiot/iothub_device_client_ll.h"
#include "azureiot/iothub_client_options.h"
#include "azureiot/iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/tlsio_cryptodev.h"

#include "azure_prov_client/iothub_security_factory.h"

#include "TobAzureHsm.h"
#include "BreakoutTrustOnboardSDK.h"

/* This sample uses the _LL APIs of iothub_client for example purposes.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
#include "azureiot/iothubtransportmqtt.h"
#endif  // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
#include "azureiot/iothubtransportmqtt_websockets.h"
#endif  // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
#include "azureiot/iothubtransportamqp.h"
#endif  // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
#include "azureiot/iothubtransportamqp_websockets.h"
#endif  // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
#include "azureiot/iothubtransporthttp.h"
#endif  // SAMPLE_HTTP

/* Change if using different device, key, baudrate or PIN */
TWILIO_TRUST_ONBOARD_HSM_CONFIG hsmConfig = {
    "/dev/ttyACM1", /* Device */
    "0000",         /* PIN code */
    0,              /* Non-zero for signing key, zero for available key */
    115200          /* Baudrate */
};

#define MESSAGE_COUNT 5
static bool g_continueRunning                    = true;
static size_t g_message_count_send_confirmations = 0;

typedef struct EVENT_INSTANCE_TAG {
  IOTHUB_MESSAGE_HANDLE messageHandle;
  size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback) {
  (void)userContextCallback;
  // When a message is sent this callback will get envoked
  g_message_count_send_confirmations++;
  (void)printf("Confirmation callback received for message %zu with result %s\r\n", g_message_count_send_confirmations,
               MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void print_usage(const char* program_name) {
  printf("%s <arguments>\n", program_name);
  printf(
      "\nRequired arguments:\n\
  -c,--connection-string=<string>    - connection string to use. Has the following format:\n\
    \"HostName=<host_name>;DeviceId=<device_id>;x509=true;UseProvisioning=true\"\n\
\nOptional arguments:\n\
  -d,--device=<device>               - serial device to connect to a SIM, /dev/ttyACM1 by default\n\
  -p,--pin=<pin>                     - PIN code for the mIAS applet on your SIM card, 0000 by default\n\
  -b,--baudrate=<baudrate>           - baud rate for a serial device. 115200 by default\n\
  -s,--signing                       - Use signing certificate instead of the available one\n");
}

int main(int argc, char** argv) {
  IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
  IOTHUB_MESSAGE_HANDLE message_handle;
  size_t messages_sent          = 0;
  const char* telemetry_msg     = "test_message";
  const char* connection_string = NULL;

  // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
  protocol = MQTT_Protocol;
#endif  // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
  protocol = MQTT_WebSocket_Protocol;
#endif  // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
  protocol = AMQP_Protocol;
#endif  // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
  protocol = AMQP_Protocol_over_WebSocketsTls;
#endif  // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
  protocol = HTTP_Protocol;
#endif  // SAMPLE_HTTP

  IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

  static struct option options[] = {
      {"device", required_argument, NULL, 'd'}, {"baudrate", required_argument, NULL, 'b'},
      {"pin", required_argument, NULL, 'p'},    {"connection-string", required_argument, NULL, 'c'},
      {"signing", no_argument, NULL, 's'},      {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:b:p:c:s", options, NULL)) != -1) {
    switch (opt) {
      case 'd':
        hsmConfig.device_path = optarg;
        break;
      case 'b': {
        long long_baudrate = strtol(optarg, NULL, 10);
        if (long_baudrate <= 0) {
          printf("Invalid baudrate: %s\n", optarg);
          print_usage(argv[0]);
          return 1;
        }
        hsmConfig.baudrate = (int)long_baudrate;
        break;
      }
      case 'p':
        hsmConfig.sim_pin = optarg;
        break;
      case 's':
        hsmConfig.signing = 1;
        break;
      case 'c':
        connection_string = optarg;
        break;
      default:
        fprintf(stderr, "Invalid option: %c\n", opt);
        print_usage(argv[0]);
        return 1;
    }
  }

  if (connection_string == NULL) {
    printf("Connection string is not set\n");
    print_usage(argv[0]);
    return 1;
  }


  // Used to initialize IoTHub SDK subsystem
  (void)IoTHub_Init();

  (void)iothub_security_init(IOTHUB_SECURITY_TYPE_X509);

  (void)printf("Creating IoTHub handle\r\n");
  // Create the iothub handle here
  device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connection_string, protocol);
  if (device_ll_handle == NULL) {
    (void)printf("Failure createing Iothub device.  Hint: Check you connection string.\r\n");
  } else {
    // Set any option that are neccessary.
    // For available options please see the iothub_sdk_options.md documentation
    // bool traceOn = true;
    // IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);

    // Set the X509 certificates in the SDK
    if (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_HSM_CONFIG_DATA, &hsmConfig) != IOTHUB_CLIENT_OK) {
      printf("Can't configure HSM, aborting\r\n");
    } else {
      do {
        if (messages_sent < MESSAGE_COUNT) {
          // Construct the iothub message from a string or a byte array
          message_handle = IoTHubMessage_CreateFromString(telemetry_msg);
          // message_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText)));

          // Set Message property
          (void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
          (void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
          (void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2Fjson");
          (void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");

          // Add custom properties to message
          (void)IoTHubMessage_SetProperty(message_handle, "property_key", "property_value");

          (void)printf("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
          IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);

          // The message is copied to the sdk so the we can destroy it
          IoTHubMessage_Destroy(message_handle);

          messages_sent++;
        } else if (g_message_count_send_confirmations >= MESSAGE_COUNT) {
          // After all messages are all received stop running
          g_continueRunning = false;
        }

        IoTHubDeviceClient_LL_DoWork(device_ll_handle);
        ThreadAPI_Sleep(1);

      } while (g_continueRunning);
    }
    // Clean up the iothub sdk handle
    IoTHubDeviceClient_LL_Destroy(device_ll_handle);
  }
  // Free all the sdk subsystem
  IoTHub_Deinit();

  return 0;
}

// Force symbol import at this point, so that the linker doesn't pull in
//   Riot and utpm implementations from Azure IoT SDK
extern const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface(void);
extern const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface(void);

const HSM_CLIENT_X509_INTERFACE* (*x509_ptr)(void) = hsm_client_x509_interface;
const HSM_CLIENT_TPM_INTERFACE* (*tpm_ptr)(void)   = hsm_client_tpm_interface;
