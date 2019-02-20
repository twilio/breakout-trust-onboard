/*
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include <azure_prov_client/hsm_client_data.h>

#include <BreakoutTrustOnboardSDK.h>

#include "custom_hsm_twilio.h"

#include "base64.h"

typedef struct TWILIO_TRUST_ONBOARD_HSM_INFO_TAG
{
    const char* device_path;
    const char* sim_pin;
    const char* certificate;
    const char* common_name;
    const char* key;
} TWILIO_TRUST_ONBOARD_HSM_INFO;

int hsm_client_x509_init()
{
    (void)printf("init-ing hsm client x509\n");
    return 0;
}

void hsm_client_x509_deinit()
{
    (void)printf("deinit-ing hsm client x509\n");
}

int hsm_client_tpm_init()
{
    return 0;
}

void hsm_client_tpm_deinit()
{
}

int subjectName(const char *cert, size_t certLen, char **subjectName)
{
  BIO* certBio = BIO_new(BIO_s_mem());
  BIO_write(certBio, cert, certLen);
  STACK_OF(X509_INFO) *certstack;
  X509_INFO *stack_item = NULL;
  X509_NAME *certsubject = NULL;
  int i;

  certstack = PEM_X509_INFO_read_bio(certBio, NULL, NULL, NULL);
  if (!certstack) {
    fprintf(stderr, "unable to parse certificate in memory\n");
    return EXIT_FAILURE;
  }

  for (i = 0; i < sk_X509_INFO_num(certstack); i++) {
    char subject_cn[256] = "** n/a **";
    long cert_version;

    stack_item = sk_X509_INFO_value(certstack, i);

    certsubject = X509_get_subject_name(stack_item->x509);
    X509_NAME_get_text_by_NID(certsubject, NID_commonName,
        subject_cn, 256);
    cert_version = (X509_get_version(stack_item->x509)+1);

    if (i == 0) {
      *subjectName = (char *)malloc(sizeof(char) * (strlen(subject_cn) + 1));
      strcpy(*subjectName, subject_cn);
    }
  }

  sk_X509_INFO_pop_free(certstack, X509_INFO_free);
  BIO_free(certBio);

  return EXIT_SUCCESS;
}

int populate_cert(TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info, const char* device_path, const char* pin)
{
  int RESULT = 0;

  (void)printf("device_path: %s, pin: %s\r\n", device_path, pin);

  if (hsm_info->certificate != nullptr) {
    return RESULT;
  }

  int ret = 0;
  uint8_t cert[CERT_BUFFER_SIZE];
  int cert_size = 0;

  tobInitialize(device_path);
  ret = tobExtractAvailablePrivateKey(cert, &cert_size, CERT_MIAS_PATH, pin);
  if (ret != 0) {
    (void)printf("Failed reading certificate\r\n");
    RESULT = 1;
  } else {
    hsm_info->certificate = (const char *)malloc(cert_size);
    memcpy((void *)(hsm_info->certificate), cert, cert_size);
  }

  (void)subjectName((const char *)cert, cert_size, (char **)&(hsm_info->common_name));
  (void)printf("Common name: %s\r\n", hsm_info->common_name);

  return RESULT;
}

int populate_key(TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info, const char* device_path, const char* pin)
{
  int RESULT = 0;

  if (hsm_info->key != nullptr) {
    return RESULT;
  }

  int ret = 0;
  uint8_t pk[PK_BUFFER_SIZE];
  int pk_size = 0;

  tobInitialize(device_path);
  ret = tobExtractAvailableCertificate(pk, &pk_size, PK_MIAS_PATH, pin);
  if (ret != 0) {
    (void)printf("Failed reading private key\r\n");
    RESULT = 1;
  } else {
    const char *key_header = "-----BEGIN RSA PRIVATE KEY-----""\n";
    const char *key_footer = "\n-----END RSA PRIVATE KEY-----";

    int expectedBase64KeyLen = Base64encode_len(pk_size); // includes 1 extra byte for \0 termination
    int base64KeyBufferSize = strlen(key_header) + expectedBase64KeyLen + strlen(key_footer);
    char *base64Key = (char *)malloc(base64KeyBufferSize); // owned by TWILIO_TRUST_ONBOARD_HSM_INFO
    char *base64KeyPtr = base64Key;

    memcpy(base64KeyPtr, key_header, strlen(key_header));
    base64KeyPtr += strlen(key_header);

    int base64EncodeRet = Base64encode(base64KeyPtr, (const char *)pk, pk_size);
    base64KeyPtr += base64EncodeRet - 1; // leave off terminating null from Base64encode

    memcpy(base64KeyPtr, key_footer, strlen(key_footer));
    base64KeyPtr += strlen(key_footer);

    *base64KeyPtr = '\0'; // terminating null

    hsm_info->key = base64Key;
    //printf("expected: %d, actual encoded: %d, final: %d, ptr_len: %d, allocated: %d, bytes: %s\r\n", expectedBase64KeyLen, base64EncodeRet, strlen(base64Key), (base64KeyPtr - base64Key), base64KeyBufferSize, base64Key);
  }

  return RESULT;
}

HSM_CLIENT_HANDLE custom_hsm_create()
{
  HSM_CLIENT_HANDLE result;
  TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO *)malloc(sizeof(TWILIO_TRUST_ONBOARD_HSM_INFO));
  memset(hsm_info, 0, sizeof(TWILIO_TRUST_ONBOARD_HSM_INFO));
  printf("in custom_hsm_create - 0x%X\n", &hsm_info);
  if (hsm_info == NULL)
  {
    (void)printf("Failed allocating hsm info\r\n");
    result = NULL;
  }
  else
  {
    result = hsm_info;
  }

  return result;
}

void custom_hsm_destroy(HSM_CLIENT_HANDLE handle)
{
    if (handle != NULL)
    {
        printf("in custom_hsm_destroy - 0x%X\n", &handle);

        TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
        // Free anything that has been allocated in this module
        if (hsm_info->device_path != nullptr) free((void *)hsm_info->device_path);
        if (hsm_info->sim_pin!= nullptr) free((void *)hsm_info->sim_pin);
        if (hsm_info->certificate != nullptr) free((void *)hsm_info->certificate);
        if (hsm_info->key != nullptr) free((void *)hsm_info->key);
        free(hsm_info);
    }
}

char* custom_hsm_get_certificate(HSM_CLIENT_HANDLE handle)
{
    printf("%s:%d in custom_hsm_get_certificate\n", __FILE__, __LINE__);
    char* result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified\r\n");
        result = NULL;
    }
    else
    {
        TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
        if (hsm_info->certificate == NULL)
        {
          if (populate_cert(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
            (void)printf("Failed reading certificate\r\n");
            result = NULL;
          }
        }
        if (hsm_info->certificate == NULL)
        {
          result = NULL;
        }
        else
        {
          // TODO: Malloc the certificate for the iothub sdk to free
          // this value will be sent unmodified to the tlsio
          // layer to be processed
          size_t len = strlen(hsm_info->certificate);
          if ((result = (char*)malloc(len + 1)) == NULL)
          {
            (void)printf("Failure allocating certificate\r\n");
            result = NULL;
          }
          else
          {
            strcpy(result, hsm_info->certificate);
          }
        }
    }
    return result;
}

char* custom_hsm_get_key(HSM_CLIENT_HANDLE handle)
{
  printf("%s:%d in custom_hsm_get_key\n", __FILE__, __LINE__);
  char* result;
  if (handle == NULL)
  {
    (void)printf("Invalid handle value specified\r\n");
    result = NULL;
  }
  else
  {
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    if (hsm_info->key == NULL)
    {
      if (populate_key(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
        (void)printf("Failed reading key\r\n");
        result = NULL;
      }
    }

    if (hsm_info->key == NULL)
    {
      result = NULL;
    }
    else
    {
      // TODO: Malloc the private key for the iothub sdk to free
      // this value will be sent unmodified to the tlsio
      // layer to be processed
      size_t len = strlen(hsm_info->key);
      if ((result = (char*)malloc(len + 1)) == NULL)
      {
        (void)printf("Failure allocating certificate\r\n");
        result = NULL;
      }
      else
      {
        strcpy(result, hsm_info->key);
      }
    }
  }
return result;
}

char* custom_hsm_get_common_name(HSM_CLIENT_HANDLE handle)
{
    char* result;
    if (handle == NULL)
    {
        (void)printf("Invalid handle value specified\r\n");
        result = NULL;
    }
    else
    {
        TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
        if (hsm_info->common_name == NULL)
        {
          if (populate_cert(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
            (void)printf("Failed reading certificate\r\n");
            result = NULL;
          }
        }
        
        if (hsm_info->common_name == NULL)
        {
          result = NULL;
        }
        else
        {
          // TODO: Malloc the common name for the iothub sdk to free
          // this value will be sent to dps
          size_t len = strlen(hsm_info->common_name);
          if ((result = (char*)malloc(len + 1)) == NULL)
          {
            (void)printf("Failure allocating certificate\r\n");
            result = NULL;
          }
          else
          {
            strcpy(result, hsm_info->common_name);
          }
        }
    }
    return result;
}

int custom_hsm_set_data(HSM_CLIENT_HANDLE handle, const void* data)
{
  int result;
  if (handle == NULL)
  {
	(void)printf("Invalid handle value specified\r\n");
	result = __LINE__;
  }
  else
  {
  	// Cast to the data and store it as necessary
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
  	TWILIO_TRUST_ONBOARD_HSM_CONFIG* config = (TWILIO_TRUST_ONBOARD_HSM_CONFIG*)data;
  	(void)printf("device: %s (%d), pin: %s (%d)\r\n", config->device_path, strlen(config->device_path), config->sim_pin, strlen(config->sim_pin));
    hsm_info->device_path = strdup(config->device_path);
    hsm_info->sim_pin = strdup(config->sim_pin);
  	result = 0;
  }
  return result;
}

// Defining the v-table for the x509 hsm calls
static const HSM_CLIENT_X509_INTERFACE x509_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    custom_hsm_get_certificate,
    custom_hsm_get_key,
    custom_hsm_get_common_name,
    custom_hsm_set_data 
};

static const HSM_CLIENT_TPM_INTERFACE tpm_interface =
{
    custom_hsm_create,
    custom_hsm_destroy,
    NULL,
    NULL,
    NULL,
    NULL,
    custom_hsm_set_data 
};

const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface()
{
    // x509 interface pointer
    return &x509_interface;
}

const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface()
{
    // tpm interface pointer
    return &tpm_interface;
}
