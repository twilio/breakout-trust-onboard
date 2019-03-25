/*
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include "MF.h"
#include <stdlib.h>
#include <string.h>

static uint8_t AID[] = {0xA0, 0x00, 0x00, 0x00, 0x87, 0x10, 0x01, 0xFF, 0x33, 0xFF, 0xFF, 0x89, 0x01, 0x01, 0x01, 0x00};

MF::MF(void) : Applet(AID, sizeof(AID)) {
}

MF::~MF(void) {
}

static bool getEFSize(uint8_t* data, uint16_t dataLen, uint16_t* size) {
  bool ret = false;
  uint16_t i, l;

  i = 0;

  // FCP Tag
  if (data[i] == 0x62) {
    i += 2;

    while (i < dataLen) {
      if (data[i] == 0x80) {
        i += 1;
        l = data[i];
        i += 1;
        *size = 0;
        while (l) {
          *size <<= 8;
          *size |= data[i];
          i++;
          l--;
        }
        ret = true;
        break;
      } else {
        i += 1;
        i += 1 + data[i];
      }
    }
  }


  return ret;
}

bool MF::verifyPin(uint8_t* pin, uint16_t pinLen) {
  uint8_t pinData[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if (pinLen > 8) {
    return false;
  }

  memcpy(pinData, pin, pinLen);

  if (transmit(0x00, 0x20, 0x00, 0x01, pinData, sizeof(pinData))) {
    if (getStatusWord() == 0x9000) {
      return true;
    }
  }

  return false;
}

bool MF::changePin(uint8_t* oldPin, uint16_t oldPinLen, uint8_t* newPin, uint16_t newPinLen) {
  uint8_t pinData[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  if ((oldPinLen > 8) || (newPinLen > 8)) {
    return false;
  }

  memcpy(&pinData[0], oldPin, oldPinLen);
  memcpy(&pinData[8], newPin, newPinLen);

  if (transmit(0x00, 0x24, 0x00, 0x01, pinData, sizeof(pinData))) {
    if (getStatusWord() == 0x9000) {
      return true;
    }
  }

  return false;
}

bool MF::readEF(uint8_t* path, uint16_t pathLen, uint8_t** data, uint16_t* dataLen) {
  if (transmit(0x00, 0xA4, 0x08, 0x04, path, pathLen, 0x00)) {
    if (getStatusWord() == 0x9000) {
      if (getEFSize(_seiface->_apduResponse, _seiface->getResponseLength(), dataLen)) {
        uint16_t i;
        uint8_t toread;

        *data = (uint8_t*)malloc((*dataLen + 1) * sizeof(uint8_t));

        for (i = 0; i < *dataLen;) {
          if ((*dataLen - i) >= 255) {
            toread = 255;
          } else {
            toread = *dataLen - i;
          }

          if (transmit(0x00, 0xB0, i >> 8, i, toread)) {
            if (getStatusWord() == 0x9000) {
              i += getResponse(&(*data)[i]);
            } else {
              return false;
            }
          } else {
            return false;
          }
        }

        (*data)[i] = '\0';
        *dataLen += 1;
        return true;
      }
    }
  }
  return false;
}

/** C Accessors	***************************************************************/

extern "C" MF* MF_create(void) {
  return new MF();
}

extern "C" void MF_destroy(MF* mf) {
  delete mf;
}

extern "C" bool MF_verify_pin(MF* mf, uint8_t* pin, uint16_t pin_len) {
  return mf->verifyPin(pin, pin_len);
}

extern "C" bool MF_change_pin(MF* mf, uint8_t* old_pin, uint16_t old_pin_len, uint8_t* new_pin, uint16_t new_pin_len) {
  return mf->changePin(old_pin, old_pin_len, new_pin, new_pin_len);
}

extern "C" bool MF_read_ef(MF* mf, uint8_t* path, uint16_t path_len, uint8_t** data, uint16_t* data_len) {
  return mf->readEF(path, path_len, data, data_len);
}

extern "C" bool MF_read_certificate(MF* mf, uint8_t** data, uint16_t* data_len) {
  return mf->readCertificate(data, data_len);
}

extern "C" bool MF_read_private_key(MF* mf, uint8_t** data, uint16_t* data_len) {
  return mf->readPrivateKey(data, data_len);
}
