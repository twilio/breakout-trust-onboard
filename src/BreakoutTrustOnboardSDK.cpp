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

static MF _mf;
static MIAS _mias;
static SEInterface* _modem = nullptr;

#define USE_BASIC_CHANNEL false

#define SE_EF_KEY_NAME_PREFIX "SE://EF/"
#define SE_MIAS_KEY_NAME_PREFIX "SE://MIAS/"
#define SE_MIAS_P11_KEY_NAME_PREFIX "SE://MIAS_P11/"

#define CERT_MIAS_PATH SE_MIAS_P11_KEY_NAME_PREFIX "CERT_AVAILABLE"
#define PK_MIAS_PATH SE_MIAS_P11_KEY_NAME_PREFIX "PRIV_AVAILABLE"

#define CERT_MIAS_PATH_LEGACY SE_MIAS_P11_KEY_NAME_PREFIX "CERT_TYPE_A"
#define PK_MIAS_PATH_LEGACY SE_MIAS_P11_KEY_NAME_PREFIX "PRIV_TYPE_A"

#define CERT_SIGNING_MIAS_PATH SE_MIAS_KEY_NAME_PREFIX "0"

typedef struct {
  char* pin;
  mias_key_pair_t* kp;
} mias_key_t;

class SelectionGuard {
 public:
  SelectionGuard(Applet& applet, bool use_basic_channel) : applet_(applet) {
    selected_ = applet_.select(use_basic_channel);
  }

  ~SelectionGuard() {
    applet_.deselect();
  }

  bool selected() {
    return selected_;
  }

 private:
  Applet& applet_;
  bool selected_;
};

static void convert_der_to_pem(char const* headerStr, uint8_t* inDer, int inDerLen, uint8_t* outPem, int* outPemLen) {
  int expectedBase64Len = Base64encode_len(inDerLen);  // includes 1 extra byte for \0 termination
  int base64EncodeRet   = -1;
  uint8_t* outPemPtr    = outPem;

  memcpy(outPemPtr, "-----BEGIN ", 11);
  outPemPtr += 11;
  memcpy(outPemPtr, headerStr, strlen(headerStr));
  outPemPtr += strlen(headerStr);
  memcpy(outPemPtr,
         "-----"
         "\n",
         6);
  outPemPtr += 6;

  base64EncodeRet = Base64encode((char*)outPemPtr, (const char*)inDer, inDerLen);
  outPemPtr += base64EncodeRet - 1;  // leave off terminating null from Base64encode

  memcpy(outPemPtr,
         "\n"
         "-----END ",
         10);
  outPemPtr += 10;
  memcpy(outPemPtr, headerStr, strlen(headerStr));
  outPemPtr += strlen(headerStr);
  memcpy(outPemPtr, "-----", 5);
  outPemPtr += 5;

  *outPemLen = (outPemPtr - outPem);

  return;
}

static int der_to_pem_len(int derLen, const char* headerStr) {
  int captionLen = strlen(headerStr);

  return 11 + captionLen + 6 +       // header
         Base64encode_len(derLen) +  // content
         9 + captionLen + 5;         // footer
};

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
  if (_mf.select(USE_BASIC_CHANNEL)) {
    if (_mf.verifyPin((uint8_t*)pin, strlen(pin))) {
      if (_mf.readEF(efname, efnamelen, data, &size)) {
        *data_size = size;
        ret        = 0;
      } else {
        ret = ERR_SE_EF_READ_OBJECT_ERROR;
      }
    } else {
      ret = ERR_SE_EF_VERIFY_PIN_ERROR;
    }
  }
  _mf.deselect();
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
  int ret = ERR_SE_MIAS_READ_OBJECT_ERROR;

  if (label == NULL) {
    return ERR_SE_MIAS_READ_OBJECT_ERROR;
  }

  if (pin == NULL) {
    ret = ERR_SE_EF_VERIFY_PIN_ERROR;
  }

#ifdef __cplusplus
  if (_mias.select(USE_BASIC_CHANNEL)) {
    if (_mias.verifyPin((uint8_t*)pin, strlen(pin))) {
      uint16_t obj_size;
      if (_mias.p11GetObjectByLabel((uint8_t*)label, strlen(label), obj, &obj_size)) {
        *size = obj_size;
        ret   = 0;
      } else {
        ret = ERR_SE_MIAS_READ_OBJECT_ERROR;
      }
    } else {
      ret = ERR_SE_EF_VERIFY_PIN_ERROR;
    }
  }
  _mias.deselect();
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
      ret = ERR_SE_EF_VERIFY_PIN_ERROR;
    }
  }
  Applet_deselect((Applet*)_mf);
#endif

  return ret;
}

int tobInitializeWithInterface(SEInterface* seiface) {
#ifdef __cplusplus
  Applet::closeAllChannels(seiface);

  _mias.init(seiface);

  _mf.init(seiface);
#else
  Applet_closeAllChannels(seiface);
  _mias = MIAS_create();
  Applet_init((Applet*)_mias, seiface);

  _mf = MF_create();
  Applet_init((Applet*)_mf, seiface);
#endif
  return 0;
}

#ifndef NO_OS
int tobInitialize(const char* device, int baudrate) {
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
    _modem = new GenericModem(device, baudrate);
  }

  if (!_modem->open()) {
    fprintf(stderr, "Error modem not found!\n");
    delete _modem;
    _modem = nullptr;
    return -1;
  }
  return tobInitializeWithInterface(_modem);
}
#endif  // NO_OS

int tob_x509_crt_extract_se(uint8_t* cert, int* cert_size, const char* path, const char* pin) {
  int ret = ERR_SE_BAD_KEY_NAME_ERROR;

  // Read Certificate from EF
  if (memcmp(path, SE_EF_KEY_NAME_PREFIX, strlen(SE_EF_KEY_NAME_PREFIX)) == 0) {
    // Remove prefix from key path
    path += strlen(SE_EF_KEY_NAME_PREFIX);
    ret = se_read_object(path, cert, cert_size, pin);
  }

  // Read Certificate from MIAS P11 Data object
  else if (memcmp(path, SE_MIAS_P11_KEY_NAME_PREFIX, strlen(SE_MIAS_P11_KEY_NAME_PREFIX)) == 0) {
    // Remove prefix from key path
    path += strlen(SE_MIAS_P11_KEY_NAME_PREFIX);
    ret = se_p11_read_object(path, cert, cert_size, pin);
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
    if (_mias.select(USE_BASIC_CHANNEL)) {
      uint16_t obj_size;

      if (_mias.getCertificateByContainerId(cid, cert, &obj_size)) {
        *cert_size = obj_size;
        ret        = 0;
      }
    }
    _mias.deselect();
#else
    if (Applet_select((Applet*)_mias, USE_BASIC_CHANNEL)) {
      uint16_t obj_size;

      if (MIAS_get_certificate_by_container_id(_mias, cid, cert, &obj_size)) {
        *cert_size = obj_size;
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
  int ret = tob_x509_crt_extract_se(cert, cert_size, CERT_MIAS_PATH, pin);
  if (ret == ERR_SE_MIAS_READ_OBJECT_ERROR) {
    ret = tob_x509_crt_extract_se(cert, cert_size, CERT_MIAS_PATH_LEGACY, pin);
  }
  return ret;
}

int tobExtractSigningCertificate(uint8_t* cert, int* cert_size, const char* pin) {
  return tob_x509_crt_extract_se(cert, cert_size, CERT_SIGNING_MIAS_PATH, pin);
}

int tobExtractSigningCertificateAsPem(uint8_t* pem_buf, int* pem_size, uint8_t* der_buf, int* der_size,
                                      const char* pin) {
  int res = tobExtractSigningCertificate(der_buf, der_size, pin);
  if (res == 0) {
    if (der_buf != nullptr && pem_buf != nullptr) {
      convert_der_to_pem("CERTIFICATE", der_buf, *der_size, pem_buf, pem_size);
    } else {
      *pem_size = der_to_pem_len(*der_size, "CERTIFICATE");
    }
  }
  return res;
}

int tobExtractAvailablePrivateKey(uint8_t* pk, int* pk_size, const char* pin) {
  int ret = tob_pk_extract_se(pk, pk_size, PK_MIAS_PATH, pin);
  if (ret == ERR_SE_MIAS_READ_OBJECT_ERROR) {
    ret = tob_pk_extract_se(pk, pk_size, PK_MIAS_PATH_LEGACY, pin);
  }
  return ret;
}

int tobExtractAvailablePrivateKeyAsPem(uint8_t* pem_buf, int* pem_size, uint8_t* der_buf, int* der_size,
                                       const char* pin) {
  int res = 0;
  res     = tobExtractAvailablePrivateKey(der_buf, der_size, pin);
  if (res == 0) {
    if (der_buf != nullptr && pem_buf != nullptr) {
      convert_der_to_pem("RSA PRIVATE KEY", der_buf, *der_size, pem_buf, pem_size);
    } else {
      *pem_size = der_to_pem_len(*der_size, "RSA PRIVATE KEY");
    }
  }
  return res;
}

int tobSigningSign(tob_algorithm_t algorithm, const uint8_t* hash, int hash_len, uint8_t* signature, int* signature_len,
                   const char* pin) {
  // TODO: do we need to deal with different paths here?
  const char* path = CERT_SIGNING_MIAS_PATH;
  if (memcmp(path, SE_MIAS_KEY_NAME_PREFIX, strlen(SE_MIAS_KEY_NAME_PREFIX)) != 0) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }
  uint8_t cid;

  // Remove prefix from key path
  path += strlen(SE_MIAS_KEY_NAME_PREFIX);

  cid = 0;
  while (*path) {
    cid *= 10;
    cid += *path - '0';
    path++;
  }

  auto sg = SelectionGuard(_mias, USE_BASIC_CHANNEL);
  if (!sg.selected()) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  mias_key_pair_t* keypair;
  if (!_mias.getKeyPairByContainerId(cid, &keypair)) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  if (!_mias.verifyPin((uint8_t*)pin, strlen(pin))) {
    return ERR_SE_EF_VERIFY_PIN_ERROR;
  }

  if (!_mias.signInit(algorithm, keypair->kid)) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  uint16_t signature_len_16;
  if (!_mias.signFinal(hash, hash_len, signature, &signature_len_16)) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  *signature_len = signature_len_16;

  return 0;
}

int tobSigningDecrypt(const uint8_t* cipher, int cipher_len, uint8_t* plain, int* plain_len, const char* pin) {
  // TODO: do we need to deal with different paths here?
  const char* path = CERT_SIGNING_MIAS_PATH;

  if (memcmp(path, SE_MIAS_KEY_NAME_PREFIX, strlen(SE_MIAS_KEY_NAME_PREFIX)) != 0) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }
  uint8_t cid;

  // Remove prefix from key path
  path += strlen(SE_MIAS_KEY_NAME_PREFIX);

  cid = 0;
  while (*path) {
    cid *= 10;
    cid += *path - '0';
    path++;
  }

  auto sg = SelectionGuard(_mias, USE_BASIC_CHANNEL);
  if (!sg.selected()) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  mias_key_pair_t* keypair;
  if (!_mias.getKeyPairByContainerId(cid, &keypair)) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  if (!_mias.verifyPin((uint8_t*)pin, strlen(pin))) {
    return ERR_SE_EF_VERIFY_PIN_ERROR;
  }

  if (!_mias.decryptInit(ALGO_RSA_PKCS1_PADDING, keypair->kid)) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  uint16_t plain_len_16;
  if (!_mias.decryptFinal(cipher, cipher_len, plain, &plain_len_16)) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  *plain_len = plain_len_16;

  return 0;
}

int tobSigningLen(const char* pin) {
  // TODO: do we need to deal with different paths here?
  const char* path = CERT_SIGNING_MIAS_PATH;

  if (memcmp(path, SE_MIAS_KEY_NAME_PREFIX, strlen(SE_MIAS_KEY_NAME_PREFIX)) != 0) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }
  uint8_t cid;

  // Remove prefix from key path
  path += strlen(SE_MIAS_KEY_NAME_PREFIX);

  cid = 0;
  while (*path) {
    cid *= 10;
    cid += *path - '0';
    path++;
  }

  auto sg = SelectionGuard(_mias, USE_BASIC_CHANNEL);
  if (!sg.selected()) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  mias_key_pair_t* keypair = nullptr;
  if (!_mias.getKeyPairByContainerId(cid, &keypair) || keypair == nullptr) {
    return ERR_SE_BAD_KEY_NAME_ERROR;
  }

  return keypair->size_in_bits / 8;
}
