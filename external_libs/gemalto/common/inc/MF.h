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

#ifndef __MF_H__
#define __MF_H__

#include "Applet.h"

#ifdef __cplusplus

class MF: public Applet {
	public:
		// Create an instance of MF.
		// Configure instance with Secure Element access interface to use.
		MF(void);
		~MF(void);
		
		// Unlock access to MF by verifying user's pin.
		// Returns true in case verify pin was successful, false otherwise.
		bool verifyPin(uint8_t* pin, uint16_t pinLen);
		
		// Change user's pin value used to unlock access to MF.
		// Returns true in case change pin was successful, false otherwise.
		bool changePin(uint8_t* oldPin, uint16_t oldPinLen, uint8_t* newPin, uint16_t newPinLen);
		
		// Read MF.
		// Path parameter is the targetted EF name.
		// Data parameter is a buffer which will contain the EF content. It will be allocated within the function.
		// Returns true in case reading was successful, false otherwise.
		bool readEF(uint8_t* path, uint16_t pathLen, uint8_t** data, uint16_t* dataLen);

		// Read Certificate.
		// Data parameter is a buffer which will contain the certificate. It will be allocated within the function.
		// Returns true in case reading was successful, false otherwise.
		bool readCertificate(uint8_t** data, uint16_t* dataLen) {
			return readEF((uint8_t*) "7FAA6F01", 8, data, dataLen);
		}
		
		// Read Private Key.
		// Data parameter is a buffer which will contain the private key. It will be allocated within the function.
		// Returns true in case reading was successful, false otherwise.
		bool readPrivateKey(uint8_t** data, uint16_t* dataLen) {
			return readEF((uint8_t*) "7FAA6F02", 8, data, dataLen);
		}
	
};

#else 
	
typedef struct MF MF; 

MF* MF_create(void);
void MF_destroy(MF* mf);
		
bool MF_verify_pin(MF* mf, uint8_t* pin, uint16_t pin_len);
bool MF_change_pin(MF* mf, uint8_t* old_pin, uint16_t old_pin_len, uint8_t* new_pin, uint16_t new_pin_len);
		
bool MF_read_ef(MF* mf, uint8_t* path, uint16_t path_len, uint8_t** data, uint16_t* data_len);

bool MF_read_certificate(MF* mf, uint8_t** data, uint16_t* data_len);
bool MF_read_private_key(MF* mf, uint8_t** data, uint16_t* data_len);

#endif

#endif /* __MF_H__ */
