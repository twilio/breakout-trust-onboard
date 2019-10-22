// Copyright (c) Microsoft. All rights reserved.
// Copyright (c) Twilio. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <azure_c_shared_utility/threadapi.h>
#include <azureiot/iothub.h>
#include <azure_c_shared_utility/crt_abstractions.h>

#include <azure_prov_client/prov_device_ll_client.h>
#include <azure_prov_client/prov_security_factory.h>
#include <azure_prov_client/prov_transport_http_client.h>

#include <azureiot/iothub_device_client_ll.h>
#include <azureiot/iothub_client_options.h>
#include <azureiot/iothub_message.h>
#include <azure_c_shared_utility/shared_util_options.h>
#include <azure_prov_client/iothub_security_factory.h>

#include "TobAzureHsm.h"
#include "BreakoutTrustOnboardSDK.h"

#include <linux/i2c-dev.h>

// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    #include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    #include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP

/* Change if using different device, key, baudrate or PIN */
TWILIO_TRUST_ONBOARD_HSM_CONFIG hsmConfig = {
  "/dev/ttyACM1", /* Device */
  "0000",         /* PIN code */
  1,              /* Non-zero for signing key, zero for available key */
  115200          /* Baudrate */
};

#define SHT35_I2C_DEVICE "/dev/i2c-1"
#define SHT35_I2C_ADDR 0x45
#define SHT35_MEASUREMENT_US 15000 // maximum measurement time is 15ms

static int sht35_device_file = -1;

static const char *global_prov_uri = "global.azure-devices-provisioning.net";

typedef struct EVENT_INSTANCE_TAG
{
  IOTHUB_MESSAGE_HANDLE messageHandle;
  size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

typedef struct TWILIO_REGISTRATION_INFO_TAG {
  char *iothub_uri;
  char *device_id;
  int registration_complete;
  int registration_success;
} TWILIO_REGISTRATION_INFO;

MU_DEFINE_ENUM_STRINGS(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);

static int sht35_init(void) {
  sht35_device_file = open(SHT35_I2C_DEVICE, O_RDWR);
  if (sht35_device_file < 0) {
    return 0;
  }

  if (ioctl(sht35_device_file, I2C_SLAVE, SHT35_I2C_ADDR) < 0) {
    return 0;
  }

  return 1;
}

static int sht35_read_data(float* temp_celsius, float* temp_farenheit, float* humidity) {
  uint8_t buf[6];

  buf[0] = 0x24;
  buf[1] = 0x00;
  write(sht35_device_file, buf, 2);
  usleep(SHT35_MEASUREMENT_US);
  int res = read(sht35_device_file, buf, 6);
  if (res != 6) {
    return 0;
  }

  uint16_t temp_raw = (buf[0] << 8) | buf[1];
  uint16_t hum_raw  = (buf[3] << 8) | buf[4];

  *temp_celsius = (175 * temp_raw) / 65535.0 - 45.0;
  *temp_farenheit = (315 * temp_raw) / 65535.0 - 49.0;
  *humidity = (100.0 * hum_raw) / 65535.0;

  return 1;
}

static void construct_message(char* msg, float temp_celsius, float temp_farenheit, float humidity, const char* deviceId) {
  sprintf(msg, "{\"deviceId\":\"%s\",\"temperatureFarenheit\":%.1f,\"temperatureCelsius\":%.1f,\"humidity\":%.1f}", deviceId, temp_farenheit, temp_celsius, humidity);
}

static char *obtain_id_scope() {
  // Allow environment to override
  const char *id_scope_env = getenv("AZURE_ID_SCOPE");
  if (id_scope_env && strcmp("", id_scope_env)) {
    return strdup(id_scope_env);
  }

  FILE *fp = fopen(AZURE_ID_SCOPE_PATH, "r");
  char buf[256];

  if (fp && (fgets(buf, 256, fp) != NULL)) {
    fclose(fp);
    buf[strcspn(buf, "\r\n")] = 0;
    return strdup(buf);
  }

  return NULL;
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char *iothub_uri, const char *device_id,
                                     void *ctx) {
  if (ctx == NULL) {
    fprintf(stderr, "ctx is NULL\r\n");
    return;
  }
  TWILIO_REGISTRATION_INFO *reg_ctx = (TWILIO_REGISTRATION_INFO *)ctx;
  if (register_result == PROV_DEVICE_RESULT_OK) {
    fprintf(stderr, "Registration Information received from service: %s!\r\n", iothub_uri);
    mallocAndStrcpy_s(&reg_ctx->iothub_uri, iothub_uri);
    mallocAndStrcpy_s(&reg_ctx->device_id, device_id);
    reg_ctx->registration_complete = 1;
    reg_ctx->registration_success = 1;
  } else {
    fprintf(stderr, "Failure encountered on registration %s\r\n",
            MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result));
    reg_ctx->registration_complete = 1;
    reg_ctx->registration_success = 0;
  }
}

static int provision_device(char** connectionString, char** deviceId) {
  char *id_scope = obtain_id_scope();
  if (!id_scope || strcmp("", id_scope) == 0) {
    printf("Azure DPS ID Scope could not be read.");
    return 0;
  }

  prov_dev_security_init(SECURE_DEVICE_TYPE_X509);
  PROV_DEVICE_LL_HANDLE handle;
  if ((handle = Prov_Device_LL_Create(global_prov_uri, id_scope, Prov_Device_HTTP_Protocol)) == NULL) {
    printf("Failed calling Prov_Device_LL_Create\n");
    prov_dev_security_deinit();
    return 0;
  }

  bool traceOn = false;
  Prov_Device_LL_SetOption(handle, PROV_HSM_CONFIG_DATA, &hsmConfig);
  Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &traceOn);

  TWILIO_REGISTRATION_INFO register_ctx;
  register_ctx.registration_complete = 0;

  if (Prov_Device_LL_Register_Device(handle, register_device_callback, &register_ctx, NULL, NULL) != PROV_DEVICE_RESULT_OK) {
    printf("Failed to start the registration process\n");
    Prov_Device_LL_Destroy(handle);
    prov_dev_security_deinit();
    return 0;
  }

  do {
    Prov_Device_LL_DoWork(handle);
    ThreadAPI_Sleep(100);
  } while (!register_ctx.registration_complete);

  Prov_Device_LL_Destroy(handle);
  prov_dev_security_deinit();

  if(!register_ctx.registration_success) {
    return 0;
  }
  /* "HostName=<host_name>;DeviceId=<device_id>;x509=true;UseProvisioning=true" */
  *connectionString = (char*) malloc(9 + strlen(register_ctx.iothub_uri) + 1 + // HostName=<host_name>;
		                     9 + strlen(register_ctx.device_id) + 1 +  // DeviceId=<device_id>;
				     5 + 4 + 1 +                               // x509=true;
				     16 + 4);                                  // UseProvisioning=true

  sprintf(*connectionString, "HostName=%s;DeviceId=%s;x509=true;UseProvisioning=true", register_ctx.iothub_uri, register_ctx.device_id);
  *deviceId = register_ctx.device_id;
  free(register_ctx.iothub_uri);
  return 1;
}

int main(void)
{
  IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
  IOTHUB_MESSAGE_HANDLE message_handle;
  char telemetry_msg[512];

  // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
  protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
  protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
  protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
  protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
  protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

  IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;

  if (!sht35_init()) {
    fprintf(stderr, "Could not initialize temperature sensor\r\n");
    return 1;
  }
  // Used to initialize IoTHub SDK subsystem
  IoTHub_Init();
  iothub_security_init(IOTHUB_SECURITY_TYPE_X509);

  char *connectionString;
  char *deviceId;
  if (!provision_device(&connectionString, &deviceId)) {
    fprintf(stderr, "Failed to provision to IoT Hub\r\n");
    IoTHub_Deinit();
    return 1;
  }

  fprintf(stderr, "Creating IoTHub handle\r\n");
  // Create the iothub handle here
  device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
  if (device_ll_handle == NULL) {
    fprintf(stderr, "Failure createing Iothub device.\r\n");
    IoTHub_Deinit();
    return 1;
  }

  if (IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_HSM_CONFIG_DATA, &hsmConfig) != IOTHUB_CLIENT_OK)
  {
    fprintf(stderr, "Can't configure HSM, aborting\r\n");
    IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    IoTHub_Deinit();
    return 1;
  }

  struct timeval ts;
  gettimeofday(&ts, NULL);
  do
  {
    struct timeval now;
    gettimeofday(&now, NULL);
    if (now.tv_sec - ts.tv_sec >= 10) {
      // Ignore tv_usec, presicion to a second is enough
      ts = now;

      float temp_celsius;
      float temp_farenheit;
      float humidity;
      int success = sht35_read_data(&temp_celsius, &temp_farenheit, &humidity);

      if (!success) {
        fprintf(stderr, "Failed to read temperature\r\n");
      } else {
        printf("Sending data:\n\tCelsius temperature: %.1f degrees\n\tFarenheit temperature: %.1f\n\tRelative humidity: %.1f%%\n\n",
                  temp_celsius, temp_farenheit, humidity);
        construct_message(telemetry_msg, temp_celsius, temp_farenheit, humidity, deviceId);
        // Construct the iothub message from a string or a byte array
        message_handle = IoTHubMessage_CreateFromString(telemetry_msg);

        // Set Message property
        IoTHubMessage_SetMessageId(message_handle, "message_1");
        IoTHubMessage_SetCorrelationId(message_handle, "correlation_1");
        IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2Fjson");
        IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");

        // Add custom properties to message
        IoTHubMessage_SetProperty(message_handle, "temperatureAlert", (temp_celsius > 28) ? "true" : "false");

        IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, NULL, NULL);

        // The message is copied to the sdk so the we can destroy it
        IoTHubMessage_Destroy(message_handle);
      }
    }

    IoTHubDeviceClient_LL_DoWork(device_ll_handle);
    ThreadAPI_Sleep(10);
  } while (1);

  IoTHubDeviceClient_LL_Destroy(device_ll_handle);
  IoTHub_Deinit();

  return 0;
}
