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

#include "SEInterface.h"

#ifdef APDU_DEBUG
#include <stdio.h>
#endif

SEInterface::SEInterface(void) {
	_apduLen = 0;
	_apduResponseLen = 0;
}

SEInterface::~SEInterface(void) {
}

bool SEInterface::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
	_apdu[APDU_CLA_OFFSET] = cla;
	_apdu[APDU_INS_OFFSET] = ins;
	_apdu[APDU_P1_OFFSET] = p1;
	_apdu[APDU_P2_OFFSET] = p2;
	_apduLen = 4;
	return transmit();
}

bool SEInterface::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le) {
	_apdu[APDU_CLA_OFFSET] = cla;
	_apdu[APDU_INS_OFFSET] = ins;
	_apdu[APDU_P1_OFFSET] = p1;
	_apdu[APDU_P2_OFFSET] = p2;
	_apdu[APDU_LE_OFFSET] = le;
	_apduLen = 5;
	return transmit();
}

bool SEInterface::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t dataLen) {
	_apdu[APDU_CLA_OFFSET] = cla;
	_apdu[APDU_INS_OFFSET] = ins;
	_apdu[APDU_P1_OFFSET] = p1;
	_apdu[APDU_P2_OFFSET] = p2;
	_apdu[APDU_LC_OFFSET] = dataLen;
	memcpy(&_apdu[APDU_DATA_OFFSET], data, dataLen);
	_apduLen = 5 + dataLen;	
	return transmit();
}

bool SEInterface::transmit(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t dataLen, uint8_t le) {
	_apdu[APDU_CLA_OFFSET] = cla;
	_apdu[APDU_INS_OFFSET] = ins;
	_apdu[APDU_P1_OFFSET] = p1;
	_apdu[APDU_P2_OFFSET] = p2;
	_apdu[APDU_LC_OFFSET] = dataLen;
	memcpy(&_apdu[APDU_DATA_OFFSET], data, dataLen);
	_apdu[5 + dataLen] = le;
	_apduLen = 5 + dataLen + 1;
	return transmit();
}

bool SEInterface::transmit(void) {
	#ifdef APDU_DEBUG
	{
	uint16_t i;
	printf("APDU SND: ");
	for(i=0; i<_apduLen; i++) {
		printf("%02X", _apdu[i]);
	}
	printf("\n");
	}
	#endif
	
	if(transmitApdu(_apdu, _apduLen, _apduResponse, &_apduResponseLen) == false) {
		return false;
	}
	
	#ifdef APDU_DEBUG
	{
	uint16_t i;
	printf("APDU RCV: ");
	for(i=0; i<_apduResponseLen; i++) {
		printf("%02X", _apduResponse[i]);
	}
	printf("\n");
	}
	#endif
	
	if((_apduResponseLen == 2) && (_apduResponse[0] == 0x6C)) {
		_apdu[4] = _apduResponse[1];
		return transmit();
	}
	
	if((_apduResponseLen == 2) && (_apduResponse[0] == 0x61)) {
		_apdu[0] = 0x00 | (_apdu[0] & 0x03);
		_apdu[1] = 0xC0;
		_apdu[2] = 0x00;
		_apdu[3] = 0x00;
		_apdu[4] = _apduResponse[1];
		_apduLen = 5;
		return transmit();
	}
	
	return true;
}

uint16_t SEInterface::getStatusWord(void) {
	uint16_t sw = 0;
	
	if(_apduResponseLen >= 2) {
		sw = (_apduResponse[_apduResponseLen - 2] << 8) | _apduResponse[_apduResponseLen - 1];
	}
	
	return sw;
}

uint16_t SEInterface::getResponse(uint8_t* data) {
	uint16_t len = 0;
	
	if(_apduResponseLen > 2) {
		len = _apduResponseLen - 2;
		if(data) {
			memcpy(data, _apduResponse, len);
		}
	}

	return len;
}

uint16_t SEInterface::getResponseLength(void) {
	uint16_t len = 0;
	
	if(_apduResponseLen > 2) {
		len = _apduResponseLen - 2;
	}

	return len;	
}

/** C Accessors	***************************************************************/

extern "C" bool SEInterface_transmit_case1(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2) {
	return seiface->transmit(cla, ins, p1, p2);
}

extern "C" bool SEInterface_transmit_case2(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t le) {
	return seiface->transmit(cla, ins, p1, p2, le);
}

extern "C" bool SEInterface_transmit_case3(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t data_len) {
	return seiface->transmit(cla, ins, p1, p2, data, data_len);
}

extern "C" bool SEInterface_transmit_case4(SEInterface* seiface, uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, uint8_t* data, uint16_t data_len, uint8_t le) {
	return seiface->transmit(cla, ins, p1, p2, data, data_len, le);
}

extern "C" uint16_t SEInterface_get_status_word(SEInterface* seiface) {
	return seiface->getStatusWord();
}

extern "C" uint16_t SEInterface_get_response(SEInterface* seiface, uint8_t* data) {
	return seiface->getResponse(data);
}

extern "C" uint16_t SEInterface_get_response_length(SEInterface* seiface) {
	return seiface->getResponseLength();
}
