/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include "Pcsc.h"

#include <memory>
#include <cstdio>

PcscSEInterface::PcscSEInterface(int reader_idx) : _idx{reader_idx} {
}
PcscSEInterface::~PcscSEInterface() {
  if (_initialized) {
    close();
  }
}

bool PcscSEInterface::open() {
  if (_initialized) {
    return true;
  }

  LONG ret;
  if ((ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &_card_context)) != SCARD_S_SUCCESS) {
    fprintf(stderr, "PCSC: Failed to initialize context: %s\n", pcsc_stringify_error(ret));
    return false;
  }

  DWORD len_readers = 0;

  if ((ret = SCardListReaders(_card_context, nullptr, nullptr, &len_readers)) != SCARD_S_SUCCESS) {
    fprintf(stderr, "PCSC: Failed to get the length of buffer for reader names: %s\n", pcsc_stringify_error(ret));
    return false;
  }

  std::unique_ptr<char> reader_names{new char[len_readers]};

  if ((ret = SCardListReaders(_card_context, nullptr, reader_names.get(), &len_readers)) != SCARD_S_SUCCESS) {
    fprintf(stderr, "PCSC: Failed to get reader names: %s\n", pcsc_stringify_error(ret));
    return false;
  }

  int pos = 0;
  char* p = nullptr;
  for (p = reader_names.get(); p != nullptr; p += strlen(p) + 1) {
    if (pos == _idx) {
      break;
    } else {
      ++pos;
    }
  }

  if (p == nullptr) {
    fprintf(stderr, "PCSC: card reader #%d is not found\n", _idx);
    return false;
  }

  if ((ret = SCardConnect(_card_context, p, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, &_card_handle, &_protocol)) !=
      SCARD_S_SUCCESS) {
    fprintf(stderr, "PCSC: could not connect to card reader: %s\n", pcsc_stringify_error(ret));
    return false;
  }

  _initialized = true;
  return true;
}

void PcscSEInterface::close() {
  SCardDisconnect(_card_handle, SCARD_LEAVE_CARD);
  SCardReleaseContext(_card_context);
  _initialized = false;
}

bool PcscSEInterface::transmitApdu(uint8_t* apdu, uint16_t apduLen, uint8_t* response, uint16_t* responseLen) {
  DWORD resp_len_long = 256 + 2;

  LONG ret;
  if ((ret = SCardTransmit(_card_handle, (_protocol == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1, apdu, apduLen,
                           nullptr, response, &resp_len_long)) != SCARD_S_SUCCESS) {
    fprintf(stderr, "PCSC: transmit failed: %s\n", pcsc_stringify_error(ret));
    return false;
  }

  if (resp_len_long > UINT16_MAX) {
    return false;
  }

  *responseLen = (uint16_t)resp_len_long;
  return true;
}

extern "C" SEInterface* PcscSEInterface_create(int reader_idx) {
  return new PcscSEInterface(reader_idx);
}

extern "C" void PcscSEInterface_destroy(SEInterface* iface) {
  delete static_cast<PcscSEInterface*>(iface);
}

extern "C" int PcscSEInterface_open(SEInterface* iface) {
  return static_cast<PcscSEInterface*>(iface)->open();
}
