/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __PCSC_SEINTERFACE_H__
#define __PCSC_SEINTERFACE_H__

#include <PCSC/winscard.h>

#include "SEInterface.h"

class PcscSEInterface : public SEInterface {
 public:
  // Create an instance of Generic Modem.
  PcscSEInterface(int reader_idx);
  ~PcscSEInterface(void);

  bool open(void) override;

  void close(void) override;

 protected:
  bool transmitApdu(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen);

 private:
  SCARDHANDLE _card_handle;
  SCARDCONTEXT _card_context;
  int _idx{0};
  DWORD _protocol{SCARD_PROTOCOL_T0};
  bool _initialized{false};
};

#endif /* __GENERIC_MODEM_H__ */
