/*
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __NET_INTERFACE_H__
#define __NET_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus

class NetInterface {
 public:
  NetInterface(void);
  virtual ~NetInterface(void);

  // Bring net interface up
  // Returns true in case operation was successful, false otherwise.
  virtual bool up(void) = 0;

  // Bring net interface down
  // Returns true in case operation was successful, false otherwise.
  virtual bool down(void) = 0;

  // Open a socket to the targetted domain name or ip address and the corresponding port.
  // Returns an handle >= 0 in case operation was successful, -1 otherwise.
  virtual int openTCPSocket(const char* address, const uint16_t port) = 0;

  // Read from a socket identify by the handle returned by previously called createSocket function.
  // Returns number of bytes read in case operation was successful, -1 otherwise.
  virtual int readSocket(int handle, uint8_t* data, uint16_t len) = 0;

  // Write to a socket identify by the handle returned by previously called createSocket function.
  // Returns number of bytes written in case operation was successful, -1 otherwise.
  virtual int writeSocket(int handle, uint8_t* data, uint16_t len) = 0;

  // Close a socket identify by the handle returned by previously called createSocket function.
  virtual void closeSocket(int handle) = 0;

  // Check if a socket identify by the handle returned by previously called createSocket function,
  // is closed.
  // Returns true in case socket is closed, false otherwise.
  virtual bool isSocketClosed(int handle) = 0;
};

#else

typedef struct NetInterface NetInterface;

bool NetInterface_up(NetInterface* netiface);
bool NetInterface_down(NetInterface* netiface);

int NetInterface_open_tcp_socket(NetInterface* netiface, const char* address, const uint16_t port);
int NetInterface_read_socket(NetInterface* netiface, int handle, uint8_t* data, uint16_t len);
int NetInterface_write_socket(NetInterface* netiface, int handle, uint8_t* data, uint16_t len);
void NetInterface_close_socket(NetInterface* netiface, int handle);

bool NetInterface_is_socket_closed(NetInterface* netiface, int handle);

#endif

#endif /* __SE_INTERFACE_H__ */
