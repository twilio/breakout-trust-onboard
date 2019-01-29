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

#include "NetInterface.h"

NetInterface::NetInterface(void) {
}

NetInterface::~NetInterface(void) {
}

/** C Accessors	***************************************************************/

extern "C" bool NetInterface_up(NetInterface* netiface) {
	return netiface->up();
}

extern "C" bool NetInterface_down(NetInterface* netiface) {
	return netiface->down();
}

extern "C" int  NetInterface_open_tcp_socket(NetInterface* netiface, const char* address, const uint16_t port) {
	return netiface->openTCPSocket(address, port);
}

extern "C" int  NetInterface_read_socket(NetInterface* netiface, int handle, uint8_t* data, uint16_t len) {
	return netiface->readSocket(handle, data, len);
}

extern "C" int  NetInterface_write_socket(NetInterface* netiface, int handle, uint8_t* data, uint16_t len) {
	return netiface->writeSocket(handle, data, len);
}

extern "C" void NetInterface_close_socket(NetInterface* netiface, int handle) {
	netiface->closeSocket(handle);
}
