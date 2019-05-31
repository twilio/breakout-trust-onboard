/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include "Applet.h"
#include "ISO7816.h"

Applet::Applet(uint8_t* aid, uint16_t aidLen) {
  _seiface    = NULL;
  _aid        = aid;
  _aidLen     = aidLen;
  _seiface    = NULL;
  _isBasic    = false;
  _isSelected = false;
  _channel    = 0;
}

Applet::~Applet(void) {
  if (isSelected()) {
    deselect();
  }
}

void Applet::closeAllChannels(SEInterface* seiface) {
  if (seiface != NULL) {
    for (int i = 1; i <= APPLET_MAX_CHANNEL; i++) {
      seiface->transmit(0x00, SCIns::ManageChannel, SCP1::MANAGECHANNELClose, static_cast<SCP2>(i), 0x01);
    }
  }
}

void Applet::init(SEInterface* seiface) {
  _seiface = seiface;
}

bool Applet::isSelected(void) {
  return _isSelected;
}

bool Applet::select(bool isBasic) {
  if (_seiface != NULL) {
    deselect();

    if (isBasic) {
      _channel = 0;

      if (_seiface->transmit(0x00, SCIns::Select, SCP1::SELECTByDFName,
                             SCP2::SELECTFCITemplate | SCP2::SELECTFirstOrOnly, _aid, _aidLen)) {
        if ((_seiface->getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) ||
            ((_seiface->getStatusWord() & 0xFF00) == SCSW1::OKLengthInSW2)) {
          _isSelected = true;
          _isBasic    = true;
          return true;
        }
      }
    } else {
      if (_seiface->transmit(0x00, SCIns::ManageChannel, SCP1::MANAGECHANNELOpen, SCP2::MANAGECHANNELAllocateChannel,
                             0x01)) {
        if (_seiface->getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
          _seiface->getResponse(&_channel);

          if (_seiface->transmit(0x00 | _channel, SCIns::Select, SCP1::SELECTByDFName,
                                 SCP2::SELECTFCITemplate | SCP2::SELECTFirstOrOnly, _aid, _aidLen)) {
            if ((_seiface->getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) ||
                ((_seiface->getStatusWord() & 0xFF00) == SCSW1::OKLengthInSW2)) {
              _isSelected = true;
              _isBasic    = false;
              return true;
            }
          }

          _seiface->transmit(0x00, SCIns::ManageChannel, SCP1::MANAGECHANNELClose, static_cast<SCP2>(_channel), 0x01);
        }
      }
    }
  }

  return false;
}

bool Applet::deselect(void) {
  if (_seiface != NULL) {
    if (_isSelected && !_isBasic) {
      if (_seiface->transmit(0x00, SCIns::ManageChannel, SCP1::MANAGECHANNELClose, static_cast<SCP2>(_channel), 0x01)) {
        if (_seiface->getStatusWord() == (SCSW1::OKNoQualification | SCSW2::OKNoQualification)) {
          _isSelected = false;
        }
      }
    } else if (_isSelected && _isBasic) {
      _isSelected = false;
    }
  }
  return _isSelected;
}

bool Applet::transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2) {
  if (_isSelected) {
    return _seiface->transmit(cla | _channel, ins, p1, p2);
  }
  return false;
}

bool Applet::transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2, uint8_t le) {
  if (_isSelected) {
    return _seiface->transmit(cla | _channel, ins, p1, p2, le);
  }
  return false;
}

bool Applet::transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2, const uint8_t* data, uint16_t dataLen) {
  if (_isSelected) {
    return _seiface->transmit(cla | _channel, ins, p1, p2, data, dataLen);
  }
  return false;
}

bool Applet::transmit(uint8_t cla, SCIns ins, SCP1 p1, SCP2 p2, const uint8_t* data, uint16_t dataLen, uint8_t le) {
  if (_isSelected) {
    return _seiface->transmit(cla | _channel, ins, p1, p2, data, dataLen, le);
  }
  return false;
}

uint16_t Applet::getStatusWord(void) {
  uint16_t sw = 0;

  if (_isSelected) {
    sw = _seiface->getStatusWord();
  }

  return sw;
}

uint16_t Applet::getResponse(uint8_t* data) {
  uint16_t len = 0;

  if (_isSelected) {
    len = _seiface->getResponse(data);
  }

  return len;
}

uint16_t Applet::getResponseLength(void) {
  uint16_t len = 0;

  if (_isSelected) {
    len = _seiface->getResponseLength();
  }

  return len;
}

/** C Accessors	***************************************************************/

extern "C" Applet* Applet_create(uint8_t* aid, uint16_t aid_len) {
  return new Applet(aid, aid_len);
}

extern "C" void Applet_destroy(Applet* applet) {
  delete applet;
}

extern "C" void Applet_closeAllChannels(SEInterface* seiface) {
  Applet::closeAllChannels(seiface);
}

extern "C" void Applet_init(Applet* applet, SEInterface* seiface) {
  applet->init(seiface);
}

extern "C" bool Applet_is_selected(Applet* applet) {
  return applet->isSelected();
}

extern "C" bool Applet_select(Applet* applet, bool is_basic) {
  return applet->select(is_basic);
}

extern "C" bool Applet_deselect(Applet* applet) {
  return applet->deselect();
}

extern "C" bool Applet_transmit_case1(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
  return applet->transmit(cla, static_cast<SCIns>(ins), static_cast<SCP1>(p1), static_cast<SCP2>(p2));
}

extern "C" bool Applet_transmit_case2(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le) {
  return applet->transmit(cla, static_cast<SCIns>(ins), static_cast<SCP1>(p1), static_cast<SCP2>(p2), le);
}

extern "C" bool Applet_transmit_case3(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                                      const uint8_t* data, uint16_t data_len) {
  return applet->transmit(cla, static_cast<SCIns>(ins), static_cast<SCP1>(p1), static_cast<SCP2>(p2), data, data_len);
}

extern "C" bool Applet_transmit_case4(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                                      const uint8_t* data, uint16_t data_len, uint8_t le) {
  return applet->transmit(cla, static_cast<SCIns>(ins), static_cast<SCP1>(p1), static_cast<SCP2>(p2), data, data_len,
                          le);
}

extern "C" uint16_t Applet_get_status_word(Applet* applet) {
  return applet->getStatusWord();
}

extern "C" uint16_t Applet_get_response(Applet* applet, uint8_t* data) {
  return applet->getResponse(data);
}

extern "C" uint16_t Applet_get_response_length(Applet* applet) {
  return applet->getResponseLength();
}
