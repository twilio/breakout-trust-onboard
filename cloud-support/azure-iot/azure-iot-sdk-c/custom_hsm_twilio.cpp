/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
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

typedef struct TWILIO_TRUST_ONBOARD_HSM_INFO_TAG {
  const char* device_path;
  const char* sim_pin;
  int signing;
  int baudrate;
  const char* certificate;
  const char* common_name;
  const char* key;
  TLSIO_CRYPTODEV_PKEY* signing_key;
} TWILIO_TRUST_ONBOARD_HSM_INFO;

int hsm_client_x509_init() {
  return 0;
}

void hsm_client_x509_deinit() {
}

int hsm_client_tpm_init() {
  return 0;
}

void hsm_client_tpm_deinit() {
}

static int subjectName(const char* cert, size_t certLen, char** subjectName) {
  BIO* certBio = BIO_new(BIO_s_mem());
  BIO_write(certBio, cert, certLen);
  STACK_OF(X509_INFO) * certstack;
  X509_INFO* stack_item  = NULL;
  X509_NAME* certsubject = NULL;
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
    X509_NAME_get_text_by_NID(certsubject, NID_commonName, subject_cn, 256);
    cert_version = (X509_get_version(stack_item->x509) + 1);

    if (i == 0) {
      *subjectName = (char*)malloc(sizeof(char) * (strlen(subject_cn) + 1));
      strcpy(*subjectName, subject_cn);
    }
  }

  sk_X509_INFO_pop_free(certstack, X509_INFO_free);
  BIO_free(certBio);

  return EXIT_SUCCESS;
}

static int populate_cert(TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info, const char* device_path, const char* pin) {
  int RESULT = 0;

  if (hsm_info->certificate != nullptr) {
    return RESULT;
  }

  int ret = 0;
  uint8_t cert[PEM_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t cert_der[DER_BUFFER_SIZE];
  int cert_der_size = 0;

  tobInitialize(device_path, hsm_info->baudrate);
  if (hsm_info->signing) {
    ret = tobExtractSigningCertificateAsPem(cert, &cert_size, cert_der, &cert_der_size, pin);
  } else {
    ret = tobExtractAvailableCertificate(cert, &cert_size, pin);
  }
  if (ret != 0) {
    (void)fprintf(stderr, "Failed reading certificate\r\n");
    RESULT = 1;
  } else {
    hsm_info->certificate = (const char*)malloc(cert_size + 1);
    memcpy((void*)(hsm_info->certificate), cert, cert_size);
    ((char*)hsm_info->certificate)[cert_size] = '\0';
  }

  (void)subjectName((const char*)cert, cert_size, (char**)&(hsm_info->common_name));

  return RESULT;
}

static int populate_key(TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info, const char* device_path, const char* pin) {
  int RESULT = 0;

  if (hsm_info->key != nullptr) {
    return RESULT;
  }

  int ret = 0;
  uint8_t pk[PEM_BUFFER_SIZE];
  int pk_size = 0;
  uint8_t pk_der[DER_BUFFER_SIZE];
  int pk_der_size = 0;

  tobInitialize(device_path, hsm_info->baudrate);
  ret = tobExtractAvailablePrivateKeyAsPem(pk, &pk_size, pk_der, &pk_der_size, pin);
  if (ret != 0) {
    (void)fprintf(stderr, "Failed reading private key\r\n");
    RESULT = 1;
  } else {
    hsm_info->key = (const char*)malloc(pk_size + 1);
    memcpy((void*)(hsm_info->key), pk, pk_size);
    ((char*)hsm_info->key)[pk_size] = '\0';
  }

  return RESULT;
}

static int hsm_signing_sign(const uint8_t* data, int datalen, uint8_t* signature, int* signature_len, void* priv) {
  TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)priv;

  if (!hsm_info) {
    return 0;
  }

  // TODO: support for other paddings and ECC
  tob_algorithm_t algo;
  switch (datalen) {
    case 20:
      algo = TOB_ALGO_SHA1_RSA_PKCS1;
      break;

    case 28:
      algo = TOB_ALGO_SHA224_RSA_PKCS1;
      break;

    case 32:
      algo = TOB_ALGO_SHA256_RSA_PKCS1;
      break;

    case 48:
      algo = TOB_ALGO_SHA384_RSA_PKCS1;
      break;

    default:
      return 0;
  }
  return (tobSigningSign(algo, data, datalen, signature, signature_len, hsm_info->sim_pin) == 0);
}

static int hsm_signing_decrypt(const uint8_t* cipher, int cipherlen, uint8_t* plain, int* plain_len, void* priv) {
  TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)priv;

  if (!hsm_info) {
    return 0;
  }

  return (tobSigningDecrypt(cipher, cipherlen, plain, plain_len, hsm_info->sim_pin) == 0);
}

static int hsm_signing_destroy(void* priv) {
  return 1;
}

static int populate_signing_key(TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info, const char* device_path, const char* pin) {
  if (hsm_info->signing_key != nullptr) {
    return 0;
  }
  hsm_info->signing_key = (TLSIO_CRYPTODEV_PKEY*)malloc(sizeof(TLSIO_CRYPTODEV_PKEY));
  if (hsm_info->signing_key == NULL) {
    return 1;
  }
  hsm_info->signing_key->sign         = hsm_signing_sign;
  hsm_info->signing_key->decrypt      = hsm_signing_decrypt;
  hsm_info->signing_key->destroy      = hsm_signing_destroy;
  hsm_info->signing_key->type         = TLSIO_CRYPTODEV_PKEY_TYPE_RSA;  // TODO: ECC support
  hsm_info->signing_key->private_data = hsm_info;

  tobInitialize(device_path, hsm_info->baudrate);

  return 0;
}

static HSM_CLIENT_HANDLE custom_hsm_create() {
  HSM_CLIENT_HANDLE result;
  TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info =
      (TWILIO_TRUST_ONBOARD_HSM_INFO*)malloc(sizeof(TWILIO_TRUST_ONBOARD_HSM_INFO));
  memset(hsm_info, 0, sizeof(TWILIO_TRUST_ONBOARD_HSM_INFO));
  if (hsm_info == NULL) {
    (void)fprintf(stderr, "Failed allocating hsm info\r\n");
    result = NULL;
  } else {
    result = hsm_info;
  }

  return result;
}

static void custom_hsm_destroy(HSM_CLIENT_HANDLE handle) {
  fprintf(stderr, "destroying custom hsm\n");
  if (handle != NULL) {
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    // Free anything that has been allocated in this module
    if (hsm_info->device_path != nullptr) free((void*)hsm_info->device_path);
    if (hsm_info->sim_pin != nullptr) free((void*)hsm_info->sim_pin);
    if (hsm_info->certificate != nullptr) free((void*)hsm_info->certificate);
    if (hsm_info->key != nullptr) free((void*)hsm_info->key);
    free(hsm_info);
  }
}

static char* custom_hsm_get_certificate(HSM_CLIENT_HANDLE handle) {
  char* result;
  if (handle == NULL) {
    (void)fprintf(stderr, "Invalid handle value specified\r\n");
    result = NULL;
  } else {
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    if (hsm_info->certificate == NULL) {
      if (populate_cert(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
        (void)fprintf(stderr, "Failed reading certificate\r\n");
        result = NULL;
      }
    }
    if (hsm_info->certificate == NULL) {
      result = NULL;
    } else {
      // TODO: Malloc the certificate for the iothub sdk to free
      // this value will be sent unmodified to the tlsio
      // layer to be processed
      size_t len = strlen(hsm_info->certificate);
      if ((result = (char*)malloc(len + 1)) == NULL) {
        (void)fprintf(stderr, "Failure allocating certificate\r\n");
        result = NULL;
      } else {
        strcpy(result, hsm_info->certificate);
      }
    }
  }
  return result;
}

static char* custom_hsm_get_key(HSM_CLIENT_HANDLE handle) {
  char* result;
  if (handle == NULL) {
    (void)fprintf(stderr, "Invalid handle value specified\r\n");
    result = NULL;
  } else {
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    if (!hsm_info->signing) {
      if (hsm_info->key == NULL) {
        if (populate_key(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
          (void)fprintf(stderr, "Failed reading key\r\n");
          result = NULL;
        }
      }

      if (hsm_info->key == NULL) {
        result = NULL;
      } else {
        // TODO: Malloc the private key for the iothub sdk to free
        // this value will be sent unmodified to the tlsio
        // layer to be processed
        size_t len = strlen(hsm_info->key);
        if ((result = (char*)malloc(len + 1)) == NULL) {
          (void)fprintf(stderr, "Failure allocating private key\r\n");
          result = NULL;
        } else {
          strcpy(result, hsm_info->key);
        }
      }
    } else {
      result = NULL;
    }
  }
  return result;
}

static TLSIO_CRYPTODEV_PKEY* custom_hsm_get_key_cryptodev(HSM_CLIENT_HANDLE handle) {
  TLSIO_CRYPTODEV_PKEY* result;
  if (handle == NULL) {
    (void)fprintf(stderr, "Invalid handle value specified\r\n");
    result = NULL;
  } else {
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    if (hsm_info->signing) {
      if (hsm_info->signing_key == NULL) {
        if (populate_signing_key(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
          (void)fprintf(stderr, "Failed reading key\r\n");
          result = NULL;
        }
      }

      result = hsm_info->signing_key;
    } else {
      result = NULL;
    }
  }
  return result;
}

static char* custom_hsm_get_common_name(HSM_CLIENT_HANDLE handle) {
  char* result;
  if (handle == NULL) {
    (void)fprintf(stderr, "Invalid handle value specified\r\n");
    result = NULL;
  } else {
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    if (hsm_info->common_name == NULL) {
      if (populate_cert(hsm_info, hsm_info->device_path, hsm_info->sim_pin) != 0) {
        (void)fprintf(stderr, "Failed reading certificate\r\n");
        result = NULL;
      }
    }

    if (hsm_info->common_name == NULL) {
      result = NULL;
    } else {
      // TODO: Malloc the common name for the iothub sdk to free
      // this value will be sent to dps
      size_t len = strlen(hsm_info->common_name);
      if ((result = (char*)malloc(len + 1)) == NULL) {
        (void)fprintf(stderr, "Failure allocating certificate\r\n");
        result = NULL;
      } else {
        strcpy(result, hsm_info->common_name);
      }
    }
  }
  return result;
}

static int custom_hsm_set_data(HSM_CLIENT_HANDLE handle, const void* data) {
  int result;
  if (handle == NULL) {
    (void)fprintf(stderr, "Invalid handle value specified\r\n");
    result = __LINE__;
  } else {
    // Cast to the data and store it as necessary
    TWILIO_TRUST_ONBOARD_HSM_INFO* hsm_info = (TWILIO_TRUST_ONBOARD_HSM_INFO*)handle;
    TWILIO_TRUST_ONBOARD_HSM_CONFIG* config = (TWILIO_TRUST_ONBOARD_HSM_CONFIG*)data;
    hsm_info->device_path                   = strdup(config->device_path);
    hsm_info->sim_pin                       = strdup(config->sim_pin);
    hsm_info->signing                       = config->signing;
    hsm_info->baudrate                      = config->baudrate;
    result                                  = 0;
  }
  return result;
}

// Defining the v-table for the x509 hsm calls
static const HSM_CLIENT_X509_INTERFACE x509_interface = {
    custom_hsm_create,  custom_hsm_destroy,           custom_hsm_get_certificate,
    custom_hsm_get_key, custom_hsm_get_key_cryptodev, custom_hsm_get_common_name,
    custom_hsm_set_data};

static const HSM_CLIENT_TPM_INTERFACE tpm_interface = {custom_hsm_create,  custom_hsm_destroy, NULL, NULL, NULL, NULL,
                                                       custom_hsm_set_data};

const HSM_CLIENT_X509_INTERFACE* hsm_client_x509_interface() {
  // x509 interface pointer
  return &x509_interface;
}

const HSM_CLIENT_TPM_INTERFACE* hsm_client_tpm_interface() {
  // tpm interface pointer
  return &tpm_interface;
}
