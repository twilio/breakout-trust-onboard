#include <stdlib.h>
#include <stdio.h>

#include <azure_c_shared_utility/threadapi.h>

#include <azureiot/iothub.h>
#include <azure_c_shared_utility/crt_abstractions.h>

#include <azure_prov_client/prov_device_ll_client.h>
#include <azure_prov_client/prov_security_factory.h>
#include <azure_prov_client/prov_transport_http_client.h>

#include "custom_hsm_twilio.h"

#include <BreakoutTrustOnboardSDK.h>

static const char* global_prov_uri = "global.azure-devices-provisioning.net";

#define MESSAGES_TO_SEND            2
#define TIME_BETWEEN_MESSAGES       2

typedef struct CLIENT_SAMPLE_INFO_TAG
{
  unsigned int sleep_time;
  char* iothub_uri;
  char* access_key_name;
  char* device_key;
  char* device_id;
  int registration_complete;
} CLIENT_SAMPLE_INFO;

typedef struct IOTHUB_CLIENT_SAMPLE_INFO_TAG
{
  int connected;
  int stop_running;
} IOTHUB_CLIENT_SAMPLE_INFO;

DEFINE_ENUM_STRINGS(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
DEFINE_ENUM_STRINGS(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
  (void)user_context;
  fprintf(stderr, "Provisioning Status: %s\r\n", ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
  if (user_context == NULL)
  {
    fprintf(stderr, "user_context is NULL\r\n");
  }
  else
  {
    CLIENT_SAMPLE_INFO* user_ctx = (CLIENT_SAMPLE_INFO*)user_context;
    if (register_result == PROV_DEVICE_RESULT_OK)
    {
      fprintf(stderr, "Registration Information received from service: %s!\r\n", iothub_uri);
      (void)mallocAndStrcpy_s(&user_ctx->iothub_uri, iothub_uri);
      (void)mallocAndStrcpy_s(&user_ctx->device_id, device_id);
      user_ctx->registration_complete = 1;
    }
    else
    {
      fprintf(stderr, "Failure encountered on registration %s\r\n", ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result) );
      user_ctx->registration_complete = 2;
    }
  }
}

void print_success(const char *iothub_uri, const char *device_id, const char *cert, const char *pk) {
  printf("{");
  printf("  status: \"SUCCESS\",\r\n");
  printf("  iothub_uri: \"%s\",\r\n", iothub_uri);
  printf("  device_id: \"%s\",\r\n", device_id);
  printf("  certificate: \"%s\",\r\n", cert);
  printf("  key: \"%s\"\r\n", pk);
  printf("}");
}

void print_failure(const char *message) {
    printf("{");
    printf("  status: \"FAILURE\",\r\n");
    printf("  message: \"%s\"\r\n", message);
    printf("}");
}

char *obtain_id_scope() {
  // Allow environment to override
  const char *id_scope_env = getenv("AZURE_ID_SCOPE");
  if (id_scope_env && strcmp("", id_scope_env)) {
    return strdup(id_scope_env);
  }

  FILE *fp = fopen(AZURE_ID_SCOPE_PATH, "r");
  char buf[256];

  if (fp && (fgets(buf, 256, fp) != NULL)) {
    fclose(fp);
    return strdup(buf);
  }

  return NULL;
}

int main(char **argc, int argv)
{
  int result = 0;

  char *id_scope = obtain_id_scope();

  SECURE_DEVICE_TYPE hsm_type = SECURE_DEVICE_TYPE_X509;

  bool traceOn = false;

  (void)IoTHub_Init();
  (void)prov_dev_security_init(hsm_type);

  PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION prov_transport = Prov_Device_HTTP_Protocol;
  CLIENT_SAMPLE_INFO user_ctx;

  if (!id_scope || strcmp("", id_scope) == 0) {
    print_failure("Azure DPS ID Scope could not be read.");
    return 1;
  }

  memset(&user_ctx, 0, sizeof(CLIENT_SAMPLE_INFO));

  // Set ini
  user_ctx.registration_complete = 0;
  user_ctx.sleep_time = 10;

  TWILIO_TRUST_ONBOARD_HSM_CONFIG* config = NULL;
  PROV_DEVICE_LL_HANDLE handle;
  if ((handle = Prov_Device_LL_Create(global_prov_uri, id_scope, prov_transport)) == NULL)
  {
    print_failure("Failed calling Prov_Device_LL_Create");
  }
  else
  {
    config = (TWILIO_TRUST_ONBOARD_HSM_CONFIG *)malloc(sizeof(TWILIO_TRUST_ONBOARD_HSM_CONFIG));
    config->device_path = strdup(MODULE_DEVICE);
    config->sim_pin = strdup(SIM_PIN);
    Prov_Device_LL_SetOption(handle, PROV_HSM_CONFIG_DATA, (void*)config);

    Prov_Device_LL_SetOption(handle, PROV_OPTION_LOG_TRACE, &traceOn);

    if (Prov_Device_LL_Register_Device(handle, register_device_callback, &user_ctx, registation_status_callback, &user_ctx) != PROV_DEVICE_RESULT_OK)
    {
      print_failure("Failed calling Prov_Device_LL_Register_Device");
    }
    else
    {
      do
      {
        Prov_Device_LL_DoWork(handle);
        ThreadAPI_Sleep(user_ctx.sleep_time);
      } while (user_ctx.registration_complete == 0);
    }
    Prov_Device_LL_Destroy(handle);
  }   

  if (user_ctx.registration_complete != 1)
  {
    print_failure("Registration failed!");
  }
  else
  {
    int ret = 0;
    uint8_t cert[PEM_BUFFER_SIZE];
    int cert_size = 0;
    uint8_t pk[PEM_BUFFER_SIZE];
    int pk_size = 0;

    tobInitialize(MODULE_DEVICE);

    ret = tobExtractAvailableCertificate(cert, &cert_size, SIM_PIN);
    if (ret != 0) {
      print_failure("Failed extracting certificate.");
      return -1;
    }

    ret = tobExtractAvailablePrivateKeyAsPem(pk, &pk_size, SIM_PIN);
    if (ret != 0) {
      print_failure("Failed extracting key.");
      return -1;
    }

    print_success(user_ctx.iothub_uri, user_ctx.device_id, cert, pk);
  }

  free(user_ctx.iothub_uri);
  free(user_ctx.device_id);
  free(id_scope);
  prov_dev_security_deinit();

  // Free all the sdk subsystem
  IoTHub_Deinit();

  return result;
}
