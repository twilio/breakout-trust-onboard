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

#include "GenericModem.h"
#include "LSerial.h"
#include <stdio.h>

//#define MODEM_DEBUG

GenericModem::GenericModem(const char * const device) : _at(new LSerial(device)) {
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
		for(i=0; i<apduLen; i++) {
			if(i && ((i % 32) == 0)) {
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
		for(i=0; i<*responseLen; i++) {
			if(i && ((i % 32) == 0)) {
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
