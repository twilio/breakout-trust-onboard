/*
 *
 * Twilio Breakout Trust Onboard SDK
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

  if (data[i] == SCTag::FileControlInfoFCP) {
    i += 2;  // ignore length, we already know it from dataLen

    // iterate over a sequence of T-L-V triples
    while (i < dataLen) {
      if (data[i] == SCTag::FCPFileSizeWithoutInfo) {
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

  if (transmit(0x00, SCIns::Verify, SCP1::VERIFYReserved, SCP2::BasicSecurityMFKey | 0x01, pinData, sizeof(pinData))) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
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

  if (transmit(0x00, SCIns::ChangeReferenceData, SCP1::CHANGEREFERENCEDATAOldAndNew, SCP2::BasicSecurityMFKey | 0x01,
               pinData, sizeof(pinData))) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      return true;
    }
  }

  return false;
}

bool MF::readEF(uint8_t* path, uint16_t pathLen, uint8_t* data, uint16_t* dataLen) {
  if (transmit(0x00, SCIns::Select, SCP1::SELECTByPathFromMF, SCP2::SELECTFCPTemplate | SCP2::SELECTFirstOrOnly, path,
               pathLen, 0x00)) {
    if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
      if (getEFSize(_seiface->_apduResponse, _seiface->getResponseLength(), dataLen)) {
        uint16_t i;
        uint8_t toread;

        // data is NULL, only return length
        if (data == NULL) {
          return true;
        }

        for (i = 0; i < *dataLen;) {
          if ((*dataLen - i) >= 255) {
            toread = 255;
          } else {
            toread = *dataLen - i;
          }

          if (transmit(0x00, SCIns::ReadBinary, i >> 8, i, toread)) {
            if (getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
              i += getResponse(&(data[i]));
            } else {
              return false;
            }
          } else {
            return false;
          }
        }

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

extern "C" bool MF_read_ef(MF* mf, uint8_t* path, uint16_t path_len, uint8_t* data, uint16_t* data_len) {
  return mf->readEF(path, path_len, data, data_len);
}

extern "C" bool MF_read_certificate(MF* mf, uint8_t* data, uint16_t* data_len) {
  return mf->readCertificate(data, data_len);
}

extern "C" bool MF_read_private_key(MF* mf, uint8_t* data, uint16_t* data_len) {
  return mf->readPrivateKey(data, data_len);
}
