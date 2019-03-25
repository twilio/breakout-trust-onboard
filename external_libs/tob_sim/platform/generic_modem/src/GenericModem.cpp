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

#include "GenericModem.h"
#include "LSerial.h"
#include <stdio.h>

//#define MODEM_DEBUG

GenericModem::GenericModem(const char* const device) : _at(new LSerial(device)) {
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
