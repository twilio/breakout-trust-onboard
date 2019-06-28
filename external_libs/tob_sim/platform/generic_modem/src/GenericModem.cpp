/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include "GenericModem.h"
#include "LSerial.h"
#include <stdio.h>

//#define MODEM_DEBUG

GenericModem::GenericModem(const char* const device, int baudrate) : _at(new LSerial(device, baudrate)) {
}

GenericModem::~GenericModem(void) {
}

bool GenericModem::transmitApdu(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen) {
  bool ret;

#ifdef MODEM_DEBUG
  // DEBBUG
  {
    uint16_t i;

    printf("[SND]: ");
    for (i = 0; i < apduLen; i++) {
      if (i && ((i % 32) == 0)) {
        printf("\r\n       ");
      }
      printf("%02X", apdu[i]);
    }
    printf("\r\n");
  }
  // -----
#endif

  ret = _at.sendATCSIM(apdu, apduLen, response, responseLen);

#ifdef MODEM_DEBUG
  // DEBBUG
  {
    uint16_t i;

    printf("[RCV]: ");
    for (i = 0; i < *responseLen; i++) {
      if (i && ((i % 32) == 0)) {
        printf("\r\n       ");
      }
      printf("%02X", response[i]);
    }
    printf("\r\n");
  }
  // -----
#endif

  return ret;
}

extern "C" SEInterface* GenericModem_create(const char* device, int baudrate) {
  return new GenericModem(device, baudrate);
}

extern "C" void GenericModem_destroy(SEInterface* iface) {
  delete static_cast<GenericModem*>(iface);
}

extern "C" int GenericModem_open(SEInterface* iface) {
  return static_cast<GenericModem*>(iface)->open();
}
