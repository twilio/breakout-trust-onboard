/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include "LSerial.h"
#include <cstdio>
#include <cstring>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>

//#define SERIAL_DEBUG

LSerial::LSerial(const char* const device) {
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
#ifdef SERIAL_DEBUG
  printf("Opening serial port...");
#endif

  int port;
  struct termios serial;

  if ((m_uart = open(_device, O_RDWR | O_NOCTTY | O_NDELAY)) >= 0) {
    tcgetattr(m_uart, &serial);

    serial.c_iflag = 0;
    serial.c_oflag = 0;
    serial.c_lflag = 0;
    serial.c_cflag = 0;

    serial.c_cc[VMIN]  = 0;
    serial.c_cc[VTIME] = 0;

    serial.c_cflag = B115200 | CS8 | CREAD;

    tcsetattr(m_uart, TCSANOW, &serial);  // Apply configuration
    fcntl(m_uart, F_SETFL, 0);

#ifdef SERIAL_DEBUG
    printf("Found serial %s %d\r\n", _device, m_uart);
#endif
    return true;
  }

  return false;
}

bool LSerial::send(char* data, unsigned long int toWrite, unsigned long int* size) {
  unsigned long int i;
  int w;

  if (m_uart < 0) {
    return false;
  }

  for (i = 0; i < toWrite;) {
    w = write(m_uart, &data[i], (toWrite - i));
    if (w == -1) {
      return false;
    } else if (w) {
      i += w;
    }
  }

  *size = toWrite;


#ifdef SERIAL_DEBUG
  if (*size) {
    unsigned long int i;
    printf("> ");
    for (i = 0; i < *size; i++) {
      if ((data[i] != '\r') && (data[i] != '\n')) {
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

  if (m_uart < 0) {
    return false;
  }

  for (i = 0; i < toRead;) {
    r = read(m_uart, &data[i], (toRead - i));
    if (r == -1) {
      return false;
    } else if (r) {
      i += r;
    }
  }

  *size = toRead;


#ifdef SERIAL_DEBUG
  if (*size) {
    unsigned long int i;
    printf("< ");
    for (i = 0; i < *size; i++) {
      if ((data[i] != '\r') && (data[i] != '\n')) {
        printf("%c", data[i]);
      }
    }
    printf("\n");
  }
#endif

  return true;
}

bool LSerial::stop(void) {
#ifdef SERIAL_DEBUG
  printf("Closing serial port...");
#endif
  if (m_uart >= 0) {
    close(m_uart);
  }
#ifdef SERIAL_DEBUG
  printf("OK\n");
#endif
  return true;
}
