/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "MF.h"
#include "MIAS.h"

#include "BreakoutTrustOnboardSDK.h"
#include "GenericModem.h"

#ifdef PCSC_SUPPORT
#include "Pcsc.h"
#endif

#include "base64.h"

static MF* _mf;
static MIAS* _mias;
static SEInterface* _modem = nullptr;

#define USE_BASIC_CHANNEL false

#define SE_EF_KEY_NAME_PREFIX "SE://EF/"
#define SE_MIAS_KEY_NAME_PREFIX "SE://MIAS/"
#define SE_MIAS_P11_KEY_NAME_PREFIX "SE://MIAS_P11/"

#define CERT_MIAS_PATH SE_MIAS_P11_KEY_NAME_PREFIX "CERT_TYPE_A"
#define PK_MIAS_PATH SE_MIAS_P11_KEY_NAME_PREFIX "PRIV_TYPE_A"

#define CERT_SIGNING_MIAS_PATH SE_MIAS_KEY_NAME_PREFIX "0"

typedef struct {
  char* pin;
  mias_key_pair_t* kp;
} mias_key_t;

static void convert_der_to_pem(char const* headerStr, uint8_t* inDer, int inDerLen, uint8_t* outPem, int *outPemLen) {
  int expectedBase64Len = Base64encode_len(inDerLen); // includes 1 extra byte for \0 termination
  int base64BufferSize = (11 + strlen((char *)headerStr) + 6) + expectedBase64Len + (9 + strlen((char *)headerStr) + 5);
  int base64EncodeRet = -1;
  uint8_t *outPemPtr = outPem;

  memcpy(outPemPtr, "-----BEGIN ", 11);
  outPemPtr += 11;
  memcpy(outPemPtr, headerStr, strlen(headerStr));
  outPemPtr += strlen(headerStr);
  memcpy(outPemPtr, "-----""\n", 6);
  outPemPtr += 6;
  
  base64EncodeRet = Base64encode((char *)outPemPtr, (const char *)inDer, inDerLen);
  outPemPtr += base64EncodeRet - 1; // leave off terminating null from Base64encode

  memcpy(outPemPtr, "\n""-----END ", 10);
  outPemPtr += 10;
  memcpy(outPemPtr, headerStr, strlen(headerStr));
  outPemPtr += strlen(headerStr);
  memcpy(outPemPtr, "-----", 5);
  outPemPtr += 5;

  *outPemPtr = '\0';

  *outPemLen = (outPemPtr - outPem);

  return;
}

static void hex_string_2_bytes_array(uint8_t* hexstr, uint16_t hexstrLen, uint8_t* bytes, uint16_t* bytesLen) {
  uint8_t d;
  uint16_t i, j;

  *bytesLen = 0;
  for (i = 0; i < hexstrLen; *bytesLen += 1) {
    d = 0;
    for (j = i + 2; i < j; i++) {
      d <<= 4;
      if ((hexstr[i] >= '0') && (hexstr[i] <= '9')) {
        d |= hexstr[i] - '0';
      } else if ((hexstr[i] >= 'a') && (hexstr[i] <= 'f')) {
        d |= hexstr[i] - 'a' + 10;
      } else if ((hexstr[i] >= 'A') && (hexstr[i] <= 'F')) {
        d |= hexstr[i] - 'A' + 10;
      }
    }
    *bytes = d;
    bytes++;
  }
}

static int se_read_ef(uint8_t* efname, uint16_t efnamelen, uint8_t* data, int* data_size, const char* pin) {
  int ret;
  uint16_t size;

  if (pin == NULL) {
    return ERR_SE_EF_VERIFY_PIN_ERROR;
  }

  *data_size = -1;

#ifdef __cplusplus
  if (_mf->select(USE_BASIC_CHANNEL)) {
    if (_mf->verifyPin((uint8_t*)pin, strlen(pin))) {
      if (_mf->readEF(efname, efnamelen, data, &size)) {
        *data_size = size;
        ret        = 0;
      } else {
        ret = ERR_SE_EF_READ_OBJECT_ERROR;
      }
    } else {
      ret = ERR_SE_EF_VERIFY_PIN_ERROR;
    }
  }
  _mf->deselect();
#else
  if (Applet_select((Applet*)_mf, USE_BASIC_CHANNEL)) {
    if (MF_verify_pin(_mf, (uint8_t*)pin, strlen(pin))) {
      if (MF_read_ef(_mf, efname, efnamelen, data, &size)) {
        *data_size = size;
        ret        = 0;
      } else {
        ret = ERR_SE_EF_READ_OBJECT_ERROR;
      }
    } else {
      ret = ERR_SE_EF_VERIFY_PIN_ERROR;
    }
  }
  Applet_deselect((Applet*)_mf);
#endif

  return ret;
}

static int se_read_object(const char* path, uint8_t* obj, int* size, const char* pin) {
  int ret;
  uint8_t efname[32];
  uint16_t efname_len;

  if (path == NULL || pin == NULL) {
    return ERR_SE_EF_INVALID_NAME_ERROR;
  }

  size_t pathlen = strlen(path);
  // Name is expected to be of even size (hex string name)
  if ((pathlen & 1) || pathlen > ((sizeof(efname) / sizeof(uint8_t)) * 2)) {
    return ERR_SE_EF_INVALID_NAME_ERROR;
  }

  efname_len = (pathlen / 2);
  hex_string_2_bytes_array((uint8_t*)path, pathlen, efname, (uint16_t*)&efname_len);
  ret = se_read_ef(efname, efname_len, obj, size, pin);

  return ret;
}

static int se_p11_read_object(const char* label, uint8_t* obj, int* size, const char* pin) {
  int ret;

  if (label == NULL || pin == NULL) {
    return ERR_SE_MIAS_READ_OBJECT_ERROR;
  }

#ifdef __cplusplus
  if (_mias->select(USE_BASIC_CHANNEL)) {
    if (_mias->verifyPin((uint8_t*)pin, strlen(pin))) {
      uint16_t obj_size;
      if (_mias->p11GetObjectByLabel((uint8_t*)label, strlen(label), obj, &obj_size)) {
        *size = obj_size;
        ret   = 0;
      } else {
        ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
      }
    } else {
      ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
    }
  }
  _mias->deselect();
#else
  if (Applet_select((Applet*)_mias, USE_BASIC_CHANNEL)) {
    if (MIAS_verify_pin(_mias, (uint8_t*)pin, strlen(pin))) {
      uint16_t obj_size;
      if (MIAS_p11_get_object_by_label(_mias, (uint8_t*)label, strlen(label), obj, &obj_size)) {
        *size = obj_size;
        ret   = 0;
      } else {
        ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
      }
    } else {
      ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
    }
  }
  Applet_deselect((Applet*)_mf);
#endif

  return ret;
}

int tob_se_init_with_interface(SEInterface* seiface) {
#ifdef __cplusplus
  Applet::closeAllChannels(seiface);

  _mias = new MIAS();
  _mias->init(seiface);

  _mf = new MF();
  _mf->init(seiface);
#else
  Applet_closeAllChannels(seiface);
  _mias = MIAS_create();
  Applet_init((Applet*)_mias, seiface);

  _mf = MF_create();
  Applet_init((Applet*)_mf, seiface);
#endif
  return 0;
}

int tobInitialize(const char* device) {
  if (_modem != nullptr) {
    return 0;
  }

  if (device == nullptr) {
    fprintf(stderr, "Error no device specified!\n");
    return -1;
  }

  if (strncmp(device, "pcsc:", 5) == 0) {
#ifdef PCSC_SUPPORT
    long idx = strtol(device + 5, 0, 10);  // device is al least 5 characters long, indexing is safe
    _modem   = new PcscSEInterface((int)idx);
#else
    fprintf(stderr, "No pcsc support, please rebuild with -DPCSC_SUPPORT=ON\n");
    return -1;
#endif
  } else {
    _modem = new GenericModem(device);
  }

  if (!_modem->open()) {
    fprintf(stderr, "Error modem not found!\n");
    delete _modem;
    _modem = nullptr;
    return -1;
  }
  return tob_se_init_with_interface(_modem);
}

int tob_x509_crt_extract_se(uint8_t* cert, int* cert_size, const char* path, const char* pin) {
  int ret = ERR_SE_BAD_KEY_NAME_ERROR;

  // Read Certificate from EF
  if (memcmp(path, SE_EF_KEY_NAME_PREFIX, strlen(SE_EF_KEY_NAME_PREFIX)) == 0) {
    // Remove prefix from key path
    path += strlen(SE_EF_KEY_NAME_PREFIX);

    if ((ret = se_read_object(path, cert, cert_size, pin)) == 0) {
      if (cert != NULL) {
        cert[*cert_size] = '\0';
      }
      *cert_size += 1;
    }
  }

  // Read Certificate from MIAS P11 Data object
  else if (memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
    // Remove prefix from key path
    path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);

    if ((ret = se_p11_read_object(path, cert, cert_size, pin)) == 0) {
      if (cert != NULL) {
        cert[*cert_size] = '\0';
      }
      *cert_size += 1;
    }
  }

  // Read Certificate from MIAS
  else if (memcmp(path, SE_MIAS_KEY_NAME_PREFIX, strlen(SE_MIAS_KEY_NAME_PREFIX)) == 0) {
    uint8_t cid;

    // Remove prefix from key path
    path += strlen(SE_MIAS_KEY_NAME_PREFIX);

    cid = 0;
    while (*path) {
      cid *= 10;
      cid += *path - '0';
      path++;
    }

#ifdef __cplusplus
    if (_mias->select(USE_BASIC_CHANNEL)) {
      uint16_t obj_size;

      if (_mias->getCertificateByContainerId(cid, cert, &obj_size)) {
        if (cert != NULL) {
          cert[obj_size] = '\0';
        }
        *cert_size = obj_size + 1;
        ret        = 0;
      }
    }
    _mias->deselect();
#else
    if (Applet_select((Applet*)_mias, USE_BASIC_CHANNEL)) {
      uint16_t obj_size;

      if (MIAS_get_certificate_by_container_id(_mias, cid, cert, &obj_size)) {
        if (cert != NULL) {
          cert[obj_size] = '\0';
        }
        *cert_size = obj_size + 1;
        ret        = 0;
      }
    }
    Applet_deselect((Applet*)_mias);
#endif
  }

  return ret;
}

int tob_pk_extract_se(uint8_t* pk, int* pk_size, const char* path, const char* pin) {
  int ret = ERR_SE_BAD_KEY_NAME_ERROR;

  // Read PKey from EF
  if (memcmp(path, SE_EF_KEY_NAME_PREFIX, strlen(SE_EF_KEY_NAME_PREFIX)) == 0) {
    // Remove prefix from key path
    path += strlen(SE_EF_KEY_NAME_PREFIX);

    if ((ret = se_read_object(path, pk, pk_size, pin)) == 0) {
      if (pk != NULL) {
        pk[*pk_size] = '\0';
      }
      *pk_size += 1;  // object is read without traling zero
    }
  }

  // Read PKey from MIAS P11 Data object
  else if (memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
    // Remove prefix from key path
    path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);

    if ((ret = se_p11_read_object(path, pk, pk_size, pin)) == 0) {
      if (pk != NULL) {
        pk[*pk_size] = '\0';
      }
      *pk_size += 1;  // object is read without traling zero
    }
  }

  return ret;
}

int tobExtractAvailableCertificate(uint8_t* cert, int* cert_size, const char* pin) {
  return tob_x509_crt_extract_se(cert, cert_size, CERT_MIAS_PATH, pin);
}

int tobExtractSigningCertificate(uint8_t* cert, int* cert_size, const char* pin) {
  uint8_t tempCert[CERT_BUFFER_SIZE];
  int extractedLen = 0;
  int res = 0;
  res = tob_x509_crt_extract_se(tempCert, &extractedLen, CERT_SIGNING_MIAS_PATH, pin);
  if (res == 0) {
    convert_der_to_pem("CERTIFICATE", (unsigned char *)tempCert, extractedLen, (unsigned char *)cert, cert_size);
  }
  return res;
}

int tobExtractAvailablePrivateKey(uint8_t* pk, int* pk_size, const char* pin) {
  return tob_pk_extract_se(pk, pk_size, PK_MIAS_PATH, pin);
}

int tobExtractAvailablePrivateKeyAsPem(uint8_t *pk, int *pk_size, const char *pin) {
  uint8_t cert[PK_BUFFER_SIZE];
  int extractedLen = 0;
  int res = 0;
  res = tob_pk_extract_se(cert, &extractedLen, PK_MIAS_PATH, pin);
  if (res == 0) {
    convert_der_to_pem("RSA PRIVATE KEY", cert, extractedLen, pk, pk_size);
  }
  return res;
}
