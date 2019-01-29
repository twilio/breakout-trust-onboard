/*
 *  Copyright (c) 2017 Gemalto Limited. All Rights Reserved
 *  This software is the confidential and proprietary information of GEMALTO.
 *  
 *  GEMALTO MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF 
 *  THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *  TO THE IMPLIED WARRANTIES OR MERCHANTABILITY, FITNESS FOR A
 *  PARTICULAR PURPOSE, OR NON-INFRINGEMENT. GEMALTO SHALL NOT BE
 *  LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS RESULT OF USING,
 *  MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.

 *  THIS SOFTWARE IS NOT DESIGNED OR INTENDED FOR USE OR RESALE AS ON-LINE
 *  CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING FAIL-SAFE
 *  PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES, AIRCRAFT
 *  NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT LIFE
 *  SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 *  SOFTWARE COULD LEAD DIRECTLY TO DEATH, PERSONAL INJURY, OR SEVERE
 *  PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH RISK ACTIVITIES"). GEMALTO
 *  SPECIFICALLY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY OF FTNESS FOR
 *  HIGH RISK ACTIVITIES;
 *
 */

#include "Applet.h"

Applet::Applet(uint8_t* aid, uint16_t aidLen) {
	_seiface = NULL;
	_aid = aid;
	_aidLen = aidLen;
	_seiface = NULL;
	_isBasic = false;
	_isSelected = false;
	_channel = 0;
}
  
Applet::~Applet(void) {
	if(isSelected()) {
		deselect();
	}
}  
  
void Applet::init(SEInterface* seiface) {
	_seiface = seiface;
}

bool Applet::isSelected(void) {
	return _isSelected;
}

bool Applet::select(bool isBasic) {	
	if(_seiface != NULL) {
		deselect();
		
		if(isBasic) {
			_channel = 0;
			
			if(_seiface->transmit(0x00, 0xA4, 0x04, 0x00, _aid, _aidLen)) {
				if((_seiface->getStatusWord() == 0x9000) || ((_seiface->getStatusWord() & 0xFF00) == 0x6100)) {
					_isSelected = true;
					_isBasic = true;
					return true;
				}
			}
		}
		else {
			if(_seiface->transmit(0x00, 0x70, 0x00, 0x00, 0x01)) {
				if(_seiface->getStatusWord() == 0x9000) {
					_seiface->getResponse(&_channel);
					
					if(_seiface->transmit(0x00 | _channel, 0xA4, 0x04, 0x00, _aid, _aidLen)) {
						if((_seiface->getStatusWord() == 0x9000) || ((_seiface->getStatusWord() & 0xFF00) == 0x6100)) {
							_isSelected = true;
							_isBasic = false;
							return true;
						}
					}
					
					_seiface->transmit(0x00, 0x70, 0x80, _channel);
				}
			}
		}	
	}
	
	return false;
}

bool Applet::deselect(void) {
	if(_seiface != NULL) {
		if(_isSelected && !_isBasic) {
			if(_seiface->transmit(0x00, 0x70, 0x80, _channel)) {
				if(_seiface->getStatusWord() == 0x9000) {
					_isSelected = false;
				}
			}
		}
		else if(_isSelected && _isBasic) {
			_isSelected = false;
		}
	}
	return _isSelected;
}

bool Applet::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
	if(_isSelected) {
		return _seiface->transmit(cla | _channel, ins, p1, p2);
	}
	return false;
}

bool Applet::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le) {
	if(_isSelected) {
		return _seiface->transmit(cla | _channel, ins, p1, p2, le);
	}
	return false;
}

bool Applet::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t dataLen) {
	if(_isSelected) {
		return _seiface->transmit(cla | _channel, ins, p1, p2, data, dataLen);
	}
	return false;
}

bool Applet::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t dataLen, uint8_t le) {
	if(_isSelected) {
		return _seiface->transmit(cla | _channel, ins, p1, p2, data, dataLen, le);
	}
	return false;
}

uint16_t Applet::getStatusWord(void) {
	uint16_t sw = 0;
	
	if(_isSelected) {
		sw = _seiface->getStatusWord();
	}
	
	return sw;
}

uint16_t Applet::getResponse(uint8_t* data) {
	uint16_t len = 0;
	
	if(_isSelected) {
		len = _seiface->getResponse(data);
	}

	return len;
}

uint16_t Applet::getResponseLength(void) {
	uint16_t len = 0;
	
	if(_isSelected) {
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
	return applet->transmit(cla, ins, p1, p2);
}

extern "C" bool Applet_transmit_case2(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le) {
	return applet->transmit(cla, ins, p1, p2, le);
}

extern "C" bool Applet_transmit_case3(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t data_len) {
	return applet->transmit(cla, ins, p1, p2, data, data_len);
}

extern "C" bool Applet_transmit_case4(Applet* applet, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t data_len, uint8_t le) {
	return applet->transmit(cla, ins, p1, p2, data, data_len, le);
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
