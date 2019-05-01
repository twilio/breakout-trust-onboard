/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __LSERIAL_H__
#define __LSERIAL_H__

#include "Serial.h"

class LSerial : public Serial {
 public:
  // Create an instance of WSerial.
  LSerial(const char* const device);
  ~LSerial(void);

  bool start(void);
  bool send(char* data, unsigned long int toWrite, unsigned long int* written);
  bool recv(char* data, unsigned long int toRead, unsigned long int* read);
  bool stop(void);

 private:
  char* _device = nullptr;
  int32_t m_uart;
};

#endif /* __LSERIAL_H__ */
