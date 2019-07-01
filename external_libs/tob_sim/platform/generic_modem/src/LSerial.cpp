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

LSerial::LSerial(const char* const device, int baudrate) {
  if (device) {
    _device = strdup(device);
  }

  _baudrate = baudrate;
  m_uart    = -1;
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

    serial.c_cflag = CS8 | CREAD;

    switch (_baudrate) {
      case 50:
        serial.c_cflag |= B50;
        break;

      case 75:
        serial.c_cflag |= B75;
        break;

      case 110:
        serial.c_cflag |= B110;
        break;

      case 134:
        serial.c_cflag |= B134;
        break;

      case 150:
        serial.c_cflag |= B150;
        break;

      case 200:
        serial.c_cflag |= B200;
        break;

      case 300:
        serial.c_cflag |= B300;
        break;

      case 600:
        serial.c_cflag |= B600;
        break;

      case 1200:
        serial.c_cflag |= B1200;
        break;

      case 1800:
        serial.c_cflag |= B1800;
        break;

      case 2400:
        serial.c_cflag |= B2400;
        break;

      case 4800:
        serial.c_cflag |= B4800;
        break;

      case 9600:
        serial.c_cflag |= B9600;
        break;

      case 19200:
        serial.c_cflag |= B19200;
        break;

      case 38400:
        serial.c_cflag |= B38400;
        break;

      case 57600:
        serial.c_cflag |= B57600;
        break;

      case 115200:
        serial.c_cflag |= B115200;
        break;

      case 230400:
        serial.c_cflag |= B230400;
        break;

#ifdef B460800
      case 460800:
        serial.c_cflag |= B460800;
        break;
#endif

#ifdef B500000
      case 500000:
        serial.c_cflag |= B500000;
        break;
#endif

#ifdef B576000
      case 576000:
        serial.c_cflag |= B576000;
        break;
#endif

#ifdef B921600
      case 921600:
        serial.c_cflag |= B921600;
        break;
#endif

#ifdef B1000000
      case 1000000:
        serial.c_cflag |= B1000000;
        break;
#endif

#ifdef B1152000
      case 11520000:
        serial.c_cflag |= B1152000;
        break;
#endif

#ifdef B1500000
      case 1500000:
        serial.c_cflag |= B1500000;
        break;
#endif

#ifdef B2000000
      case 2000000:
        serial.c_cflag |= B2000000;
        break;
#endif

#ifdef B2500000
      case 2500000:
        serial.c_cflag |= B2500000;
        break;
#endif

#ifdef B3000000
      case 3000000:
        serial.c_cflag |= B3000000;
        break;
#endif

#ifdef B3500000
      case 3500000:
        serial.c_cflag |= B3500000;
        break;
#endif

#ifdef B4000000
      case 4000000:
        serial.c_cflag |= B4000000;
        break;
#endif

      default:
        close(m_uart);
        m_uart = -1;
        return false;
    }

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
