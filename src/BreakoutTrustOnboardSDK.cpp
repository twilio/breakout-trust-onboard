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
#include <string.h>

#include "MF.h"
#include "MIAS.h"

#include "BreakoutTrustOnboardSDK.h"
#include "GenericModem.h"

#include "SEInterface.h"

static MF* _mf;
static MIAS* _mias;
static GenericModem* _modem = nullptr;

#define USE_BASIC_CHANNEL false

#define SE_EF_KEY_NAME_PREFIX "SE://EF/"
#define SE_MIAS_KEY_NAME_PREFIX "SE://MIAS/"
#define SE_MIAS_P11_KEY_NAME_PREFIX "SE://MIAS_P11/"

#define CERT_MIAS_PATH SE_MIAS_P11_KEY_NAME_PREFIX "CERT_TYPE_A"
#define PK_MIAS_PATH SE_MIAS_P11_KEY_NAME_PREFIX "PRIV_TYPE_A"

typedef struct {
  char* pin;
  mias_key_pair_t* kp;
} mias_key_t;

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

static int se_read_ef(uint8_t* efname, uint16_t efnamelen, char** data, int* data_size, const char* pin) {
  int ret;
  uint16_t size;

  if (pin == NULL) {
    return ERR_SE_EF_VERIFY_PIN_ERROR;
  }

  *data_size = -1;

#ifdef __cplusplus
  if (_mf->select(USE_BASIC_CHANNEL)) {
    if (_mf->verifyPin((uint8_t*)pin, strlen(pin))) {
      if (_mf->readEF(efname, efnamelen, (uint8_t**)data, &size)) {
        *data_size = size & 0x0000FFFF;
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
      if (MF_read_ef(_mf, efname, efnamelen, (uint8_t**)data, &size)) {
        *data_size = size & 0x0000FFFF;
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

static int se_read_object(const char* path, char** obj, int* size, const char* pin) {
  int ret;
  uint8_t* efname;
  uint16_t efname_len;

  if (path == NULL || pin == NULL) {
    return ERR_SE_EF_INVALID_NAME_ERROR;
  }

  // Name is expected to be of even size (hex string name)
  if (strlen(path) & 1) {
    return ERR_SE_EF_INVALID_NAME_ERROR;
  }

  efname_len = (strlen(path) / 2);
  efname     = (uint8_t*)malloc(efname_len * sizeof(uint8_t));
  hex_string_2_bytes_array((uint8_t*)path, strlen(path), efname, (uint16_t*)&efname_len);
  ret = se_read_ef(efname, efname_len, obj, size, pin);
  free(efname);

  return ret;
}

static int se_p11_read_object(const char* label, char** obj, int* size, const char* pin) {
  int ret;

  if (label == NULL || pin == NULL) {
    return ERR_SE_MIAS_READ_OBJECT_ERROR;
  }

#ifdef __cplusplus
  if (_mias->select(USE_BASIC_CHANNEL)) {
    if (_mias->verifyPin((uint8_t*)pin, strlen(pin))) {
      if (_mias->p11GetObjectByLabel((uint8_t*)label, strlen(label), (uint8_t**)obj, (uint16_t*)size)) {
        *size = *size & 0x0000FFFF;
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
      if (MIAS_p11_get_object_by_label(_mias, (uint8_t*)label, strlen(label), (uint8_t**)obj, (uint16_t*)size)) {
        *size = *size & 0x0000FFFF;
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
  _mias = new MIAS();
  _mias->init(seiface);

  _mf = new MF();
  _mf->init(seiface);
#else
  _mias = MIAS_create();
  Applet_init((Applet*)_mias, seiface);

  _mf = MF_create();
  Applet_init((Applet*)_mf, seiface);
#endif
  return 0;
}

int tobInitialize(const char* device) {
  if (_modem != nullptr) {
    //		printf("Modem already setup, skipping initialization.\n");
    return 0;
  }

  if (device == nullptr) {
    printf("Error no device specified!\n");
    return -1;
  }

  _modem = new GenericModem(device);

  if (!_modem->open()) {
    printf("Error modem not found!\n");
    free(_modem);
    _modem = nullptr;
    return -1;
  }
  return tob_se_init_with_interface(_modem);
}

int tob_x509_crt_extract_se(uint8_t* cert, int* cert_size, const char* path, const char* pin) {
  int ret = ERR_SE_BAD_KEY_NAME_ERROR;

  // Read Certificate from EF
  if (memcmp(path, SE_EF_KEY_NAME_PREFIX, strlen(SE_EF_KEY_NAME_PREFIX)) == 0) {
    int obj_size;
    char* obj = NULL;

    // Remove prefix from key path
    path += strlen(SE_EF_KEY_NAME_PREFIX);

    if ((ret = se_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(cert, obj, obj_size);
      cert[obj_size] = '\0';
      *cert_size     = obj_size;
    }

    if (obj != NULL) {
      free(obj);
    }
  }

  // Read Certificate from MIAS P11 Data object
  else if (memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
    int obj_size;
    char* obj = NULL;

    // Remove prefix from key path
    path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);

    if ((ret = se_p11_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(cert, obj, obj_size);
      cert[obj_size] = '\0';
      *cert_size     = obj_size;
    }

    if (obj != NULL) {
      free(obj);
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
      char* obj;
      int obj_size;

      if (_mias->getCertificateByContainerId(cid, (uint8_t**)&obj, (uint16_t*)&obj_size)) {
        obj_size &= 0x0000FFFF;
        memcpy(cert, obj, obj_size);
        cert[obj_size] = '\0';
        *cert_size     = obj_size;
        free(obj);
      }
    }
    _mias->deselect();
#else
    if (Applet_select((Applet*)_mias, USE_BASIC_CHANNEL)) {
      char* obj;
      int obj_size;

      if (MIAS_get_certificate_by_container_id(_mias, cid, (uint8_t**)&obj, (uint16_t*)&obj_size)) {
        obj_size &= 0x0000FFFF;
        if (obj_size > 0) {
          memcpy(cert, obj, obj_size);
          cert[obj_size] = '\0';
          *cert_size     = obj_size;
          free(obj);
        }
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
    int obj_size;
    char* obj = NULL;

    // Remove prefix from key path
    path += strlen(SE_EF_KEY_NAME_PREFIX);

    if ((ret = se_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(pk, obj, obj_size);
      pk[obj_size] = '\0';
      *pk_size     = obj_size;
    }

    if (obj != NULL) {
      free(obj);
    }
  }

  // Read PKey from MIAS P11 Data object
  else if (memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
    int obj_size;
    char* obj = NULL;

    // Remove prefix from key path
    path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);

    if ((ret = se_p11_read_object(path, &obj, &obj_size, pin)) == 0) {
      memcpy(pk, obj, obj_size);
      pk[obj_size] = '\0';
      *pk_size     = obj_size;
    }

    if (obj != NULL) {
      free(obj);
    }
  }

  return ret;
}

int tobExtractAvailableCertificate(uint8_t* cert, int* cert_size, const char* pin) {
  return tob_x509_crt_extract_se(cert, cert_size, CERT_MIAS_PATH, pin);
}

int tobExtractAvailablePrivateKey(uint8_t* pk, int* pk_size, const char* pin) {
  return tob_pk_extract_se(pk, pk_size, PK_MIAS_PATH, pin);
}
