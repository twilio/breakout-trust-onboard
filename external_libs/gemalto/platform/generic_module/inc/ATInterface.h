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
