/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __AT_INTERFACE_H__
#define __AT_INTERFACE_H__

#include "Serial.h"

class ATInterface {
 public:
  ATInterface(Serial* serial);
  virtual ~ATInterface(void);

  bool open(void);
  void close(void);

  bool sendATCSIM(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen);

 protected:
  bool bytesArray2HexString(uint8_t* bytes, uint16_t bytesLen, uint8_t* hexstr, uint16_t* hexstrLen);
  bool hexString2BytesArray(uint8_t* hexstr, uint16_t hexstrLen, uint8_t* bytes, uint16_t* bytesLen);
  bool readLine(char* data, unsigned long int* len);

 private:
  Serial* _serial;
};

#endif /* __AT_INTERFACE_H__ */
