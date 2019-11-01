#include <stdlib.h>
#include <stdio.h>

#include <getopt.h>

#include <azure_c_shared_utility/threadapi.h>

#include <azureiot/iothub.h>
#include <azure_c_shared_utility/crt_abstractions.h>

#include <azure_prov_client/prov_device_ll_client.h>
#include <azure_prov_client/prov_security_factory.h>
#include <azure_prov_client/prov_transport_http_client.h>
#include <json.hpp>

#include "TobAzureHsm.h"

#include "BreakoutTrustOnboardSDK.h"
//

static const char *global_prov_uri = "global.azure-devices-provisioning.net";

#define MAX_BAUDRATE 4000000

#define MESSAGES_TO_SEND 2
#define TIME_BETWEEN_MESSAGES 2

typedef struct CLIENT_SAMPLE_INFO_TAG {
  unsigned int sleep_time;
  char *iothub_uri;
  char *access_key_name;
  char *device_key;
  char *device_id;
  int registration_complete;
} CLIENT_SAMPLE_INFO;

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG {
  int connected;
  int stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

MU_DEFINE_ENUM_STRINGS(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
MU_DEFINE_ENUM_STRINGS(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void *user_context) {
  (void)user_context;
  fprintf(stderr, "Provisioning Status: %s\r\n", MU_ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char *iothub_uri, const char *device_id,
                                     void *user_context) {
  if (user_context == NULL) {
    fprintf(stderr, "user_context is NULL\r\n");
  } else {
    CLIENT_SAMPLE_INFO *user_ctx = (CLIENT_SAMPLE_INFO *)user_context;
    if (register_result == PROV_DEVICE_RESULT_OK) {
      fprintf(stderr, "Registration Information received from service: %s!\r\n", iothub_uri);
      (void)mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
      (void)mallocAndStrcpy_s(&user_ctx->device_id, device_id);
      user_ctx->registration_complete = 1;
    } else {
      fprintf(stderr, "Failure encountered on registration %s\r\n",
              MU_ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result));
      user_ctx->registration_complete = 2;
    }
  }
}

void print_success(const char *iothub_uri, const char *device_id) {
  nlohmann::json j;

  j["status"] = "SUCCESS";
  j["iothub_uri"] = std::string(iothub_uri);
  j["device_id"] = std::string(device_id);

  std::cout << j.dump() << std::endl;
}

void print_failure(const char *message) {
  nlohmann::json j;

  j["status"] = "FAILURE";
  j["message"] = std::string(message);

  std::cout << j.dump() << std::endl;
}

void print_usage(const char* program_name) {
  fprintf(stderr, "%s <arguments>\n", program_name);
  fprintf(stderr,
          "\nRequired arguments:\n\
  -d,--device=<device>              - device to connect to a SIM, can be either a\n\
                                      char device under /dev or of form pcsc:N if\n\
				      built with PC/SC interface support\n\
  -p,--pin=<pin>                    - PIN code for the mIAS applet on your SIM card\n\
  -k,--keypair=<signing|available>  - TLS pair to use, signing or available\n\
  -a,--azure-scope=<scope>          - Azure ID scope\n\
\n\
Optional arguments:\n\
  -b,--baudrate=<baudrate>          - baud rate for a serial device. Defaults to\n\
                                      115200\n\
  -v,--verbose                      - enable verbose (tracing) output\n\
\n\
Examples:\n");
  fprintf(stderr, "\n%s -d /dev/ttyACM1 -p 0000 -b 115200 -a 0neC0FFEE\n", program_name);
#ifdef PCSC_SUPPORT
  fprintf(stderr, "\n%s --device=pcsc:0 --pin=0000 --azure-scope=0neC0FFEE\n", program_name);
#endif
}

int main(int argc, char **argv) {
  int result = 0;

  char* device            = NULL;
  int baudrate            = 115200;
  char* pin               = NULL;
  char *id_scope          = NULL;
  bool trace              = false;
  char* keypair           = NULL;
  bool signing            = false;

  static struct option options[] = {{"device", required_argument, NULL, 'd'},
                                    {"baudrate", required_argument, NULL, 'b'},
                                    {"pin", required_argument, NULL, 'p'},
                                    {"keypair", required_argument, NULL, 'k'},
                                    {"azure-scope", required_argument, NULL, 'a'},
                                    {"verbose", no_argument, NULL, 'v'},
                                    {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:b:p:k:a:v", options, NULL)) != -1) {
    switch (opt) {
      case 'd':
        device = optarg;
        break;
      case 'b': {
        long long_baudrate = strtol(optarg, NULL, 10);
        if (long_baudrate < 0 || long_baudrate >= MAX_BAUDRATE) {
          fprintf(stderr, "Invalid baudrate: %s\n", optarg);
          print_usage(argv[0]);
          return 1;
        }
        baudrate = (int)long_baudrate;
        break;
      }
      case 'p':
        pin = optarg;
        break;
      case 'a':
        id_scope = optarg;
        break;
      case 'k':
        keypair = optarg;
        break;
      case 'v':
        trace = true;
        break;
      default:
        fprintf(stderr, "Invalid option: %c\n", opt);
        print_usage(argv[0]);
        return 1;
    }
  }

  if (device == NULL) {
    fprintf(stderr, "Device is not set\n");
    print_usage(argv[0]);
    return 1;
  }


  if (pin == NULL) {
    fprintf(stderr, "PIN is not set\n");
    print_usage(argv[0]);
    return 1;
  }

  if (keypair == NULL) {
    fprintf(stderr, "Keypair is not set\n");
    print_usage(argv[0]);
    return 1;
  } else if (strcmp(keypair, "signing") == 0) {
    signing = true;
  } else if (strcmp(keypair, "available") == 0) {
    signing = false;
  } else {
    fprintf(stderr, "Unexpected keypair value: %s\n", keypair);
    print_usage(argv[0]);
    return 1;
  }

  if (id_scope == NULL) {
    fprintf(stderr, "Azure ID scope is not set\n");
    print_usage(argv[0]);
    return 1;
  }

  SECURE_DEVICE_TYPE hsm_type = SECURE_DEVICE_TYPE_X509;

  (void)IoTHub_Init();
  (void)prov_dev_security_init(hsm_type);

  PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport = Prov_Device_HTTP_Protocol;
  CLIENT_SAMPLE_INFO user_ctx;

  if (!id_scope || strcmp("", id_scope) == 0) {
    print_failure("Azure DPS ID Scope could not be read.");
    return 1;
  }

  memset(&user_ctx, 0, sizeof(CLIENT_SAMPLE_INFO));

  user_ctx.registration_complete = 0;
  user_ctx.sleep_time            = 10;

  TWILIO_TRUST_ONBOARD_HSM_CONFIG *config = NULL;
  PROV_DEVICE_LL_HANDLE handle;
  if ((handle = Prov_Device_LL_Create(global_prov_uri, id_scope, prov_transport)) == NULL) {
    print_failure("Failed calling Prov_Device_LL_Create");
  } else {
    config              = (TWILIO_TRUST_ONBOARD_HSM_CONFIG *)malloc(sizeof(TWILIO_TRUST_ONBOARD_HSM_CONFIG));
    config->device_path = strdup(device);
    config->baudrate    = baudrate;
    config->sim_pin     = strdup(pin);
    config->signing     = signing;

    Prov_Device_LL_SetOption(handle, PROV_HSM_CONFIG_DATA, (void *)config);

    Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &trace);

    if (Prov_Device_LL_Register_Device(handle, register_device_callback, &user_ctx, registation_status_callback,
                                       &user_ctx) != PROV_DEVICE_RESULT_OK) {
      print_failure("Failed calling Prov_Device_LL_Register_Device");
    } else {
      do {
        Prov_Device_LL_DoWork(handle);
        ThreadAPI_Sleep(user_ctx.sleep_time);
      } while (user_ctx.registration_complete == 0);
    }
    Prov_Device_LL_Destroy(handle);
  }

  if (user_ctx.registration_complete != 1) {
    print_failure("Registration failed!");
  } else {
    print_success(user_ctx.iothub_uri, user_ctx.device_id);
  }

  free(user_ctx.iothub_uri);
  free(user_ctx.device_id);
  prov_dev_security_deinit();

  // Free all the sdk subsystem
  IoTHub_Deinit();

  return result;
}

// Force symbol import at this point, so that the linker doesn't pull in
//   Riot and utpm implementations from Azure IoT SDK
extern const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface(void);
extern const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface(void);

const HSM_CLIENT_X509_INTERFACE* (*x509_ptr)(void) = hsm_client_x509_interface;
const HSM_CLIENT_TPM_INTERFACE* (*tpm_ptr)(void) = hsm_client_tpm_interface;
