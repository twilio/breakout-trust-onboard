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

#include "LSerial.h"
#include <cstdio>
#include <cstring>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

//#define SERIAL_DEBUG

LSerial::LSerial(const char * const device) {
	if (device) {
		_device = strdup(device);
	}
	m_uart = -1;
}

LSerial::~LSerial(void) {
	if (_device) {
		free(_device);
	}
}


bool LSerial::start(void) {
	printf("Opening serial port...");

	int port;
	struct termios serial;

	if((m_uart = open(_device, O_RDWR | O_NOCTTY | O_NDELAY)) >= 0) {
		tcgetattr(m_uart, &serial);

		serial.c_iflag = 0;
		serial.c_oflag = 0;
		serial.c_lflag = 0;
		serial.c_cflag = 0;

		serial.c_cc[VMIN] = 0;
		serial.c_cc[VTIME] = 0;

		serial.c_cflag = B115200 | CS8 | CREAD;

		tcsetattr(m_uart, TCSANOW, &serial); // Apply configuration
		fcntl(m_uart, F_SETFL, 0);

		printf("Found serial %s %d\r\n", _device, m_uart);
		return true;
	}
	
	return false;
}

bool LSerial::send(char* data, unsigned long int toWrite, unsigned long  int* size) {
	unsigned long int i;
	int w;
	
	if(m_uart < 0) {
		return false;
	}
	
	for(i=0; i<toWrite;) {
		w = write(m_uart, &data[i], (toWrite - i));
		if(w == -1) {
			return false;
		}
		else if(w) {
			i += w;
		}
		
	}

	*size = toWrite;

	
	#ifdef SERIAL_DEBUG
	if(*size) {
		unsigned long int i;
		printf("> ");
		for(i=0; i<*size; i++) {
			if((data[i] != '\r') && (data[i] != '\n')) {
				printf("%c", data[i]);
			}
		}
		printf("\n");

	}
	#endif
	
	return true;
}

bool LSerial::recv(char* data, unsigned long int toRead, unsigned long int* size) {
	unsigned long int i;
	int r;
	
	if(m_uart < 0) {
		return false;
	}
	
	for(i=0; i<toRead;) {
		r = read(m_uart, &data[i], (toRead - i));
		if(r == -1) {
			return false;
		}
		else if(r) {
			i += r;
		}
		
	}

	*size = toRead;

	
	#ifdef SERIAL_DEBUG
	if(*size) {
		unsigned long int i;
		printf("< ");
		for(i=0; i<*size; i++) {
			if((data[i] != '\r') && (data[i] != '\n')) {
				printf("%c", data[i]);
			}
		}
		printf("\n");

	}
	#endif
	
	return true;
}

bool LSerial::stop(void) {
	printf("Closing serial port...");
	if(m_uart >= 0) {
		close(m_uart);
	}
	printf("OK\n");
	return true;
}

