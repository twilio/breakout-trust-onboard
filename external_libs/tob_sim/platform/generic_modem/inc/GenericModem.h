/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __GENERIC_MODEM_H__
#define __GENERIC_MODEM_H__

#include "ATInterface.h"
#include "SEInterface.h"

#ifdef __cplusplus

class GenericModem : public SEInterface {
 public:
  // Create an instance of Generic Modem.
  GenericModem(const char* device, int baudrate = 115200);
  ~GenericModem(void);

  bool open(void) override {
    return _at.open();
  }

  void close(void) override {
    _at.close();
  }

 protected:
  bool transmitApdu(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen) override;

 private:
  ATInterface _at;
};

#else /* __cplusplus */

SEInterface* GenericModem_create(const char* device, int baudrate);
void GenericModem_destroy(SEInterface* iface);
int GenericModem_open(SEInterface* iface);

#endif /* __cplusplus */

#endif /* __GENERIC_MODEM_H__ */
