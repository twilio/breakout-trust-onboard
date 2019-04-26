/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __SE_INTERFACE_H__
#define __SE_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ISO7816.h"

#define APDU_CLA_OFFSET 0
#define APDU_INS_OFFSET 1
#define APDU_P1_OFFSET 2
#define APDU_P2_OFFSET 3
#define APDU_LE_OFFSET 4
#define APDU_LC_OFFSET 4
#define APDU_DATA_OFFSET 5

//#define APDU_DEBUG

#ifdef __cplusplus

class SEInterface {
 public:
  SEInterface(void);
  virtual ~SEInterface(void);

  virtual bool open(void)  = 0;
  virtual void close(void) = 0;

  // Transmit an APDU case 1
  // Returns true in case transmit was successful, false otherwise.
  bool transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2);
  bool transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2) {
    return transmit(cla, static_cast<uint8_t>(ins), static_cast<uint8_t>(p1), static_cast<uint8_t>(p2));
  }

  // Transmit an APDU case 2
  // Returns true in case transmit was successful, false otherwise.
  bool transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le);
  bool transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2, uint8_t le) {
    return transmit(cla, static_cast<uint8_t>(ins), static_cast<uint8_t>(p1), static_cast<uint8_t>(p2), le);
  }

  // Transmit an APDU case 3
  // Returns true in case transmit was successful, false otherwise.
  bool transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const uint8_t* data, uint16_t dataLen);
  bool transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2, const uint8_t* data, uint16_t dataLen) {
    return transmit(cla, static_cast<uint8_t>(ins), static_cast<uint8_t>(p1), static_cast<uint8_t>(p2), data, dataLen);
  }

  // Transmit an APDU case 4
  // Returns true in case transmit was successful, false otherwise.
  bool transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const uint8_t* data, uint16_t dataLen, uint8_t le);
  bool transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2, const uint8_t* data, uint16_t dataLen, uint8_t le) {
    return transmit(cla, static_cast<uint8_t>(ins), static_cast<uint8_t>(p1), static_cast<uint8_t>(p2), data, dataLen,
                    le);
  }

  // Returns the status word received after the last successful transmit, 0 otherwise.
  uint16_t getStatusWord(void);

  // Copy the data response received after the last successful transmit and returns its length, 0 otherwise.
  uint16_t getResponse(uint8_t* data);

  // Returns the length of the data response received after the last successful transmit, 0 otherwise.
  uint16_t getResponseLength(void);

  // Internal buffers
  uint8_t _apdu[5 + 256 + 1];
  uint16_t _apduLen;
  uint8_t _apduResponse[2 + 256];
  uint16_t _apduResponseLen;

 protected:
  // Low layer implementation to transmit an APDU and retrieve the corresponding APDU Response
  // Returns true in case transmit was successful, false otherwise
  virtual bool transmitApdu(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen) = 0;

 private:
  // 'In between' layer implementation which auto handle 6Cxx and 61xx response
  // Stack:
  //  - transmitApdu
  //  - transmit
  //  - transmit (case 1 ... 4)
  // Returns true in case transmit was successful, false otherwise
  bool transmit(void);
};

#else

typedef struct SEInterface SEInterface;

bool SEInterface_lock(SEInterface* seiface);
bool SEInterface_unlock(SEInterface* seiface);

bool SEInterface_transmit_case1(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2);
bool SEInterface_transmit_case2(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le);
bool SEInterface_transmit_case3(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                                const uint8_t* data, uint16_t data_len);
bool SEInterface_transmit_case4(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                                const uint8_t* data, uint16_t data_len, uint8_t le);

uint16_t SEInterface_get_status_word(SEInterface* seiface);
uint16_t SEInterface_get_response(SEInterface* seiface, uint8_t* data);
uint16_t SEInterface_get_response_length(SEInterface* seiface);

#endif

#endif /* __SE_INTERFACE_H__ */
