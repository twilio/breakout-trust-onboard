/*
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
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

extern "C" int NetInterface_open_tcp_socket(NetInterface* netiface, const char* address, const uint16_t port) {
  return netiface->openTCPSocket(address, port);
}

extern "C" int NetInterface_read_socket(NetInterface* netiface, int handle, uint8_t* data, uint16_t len) {
  return netiface->readSocket(handle, data, len);
}

extern "C" int NetInterface_write_socket(NetInterface* netiface, int handle, uint8_t* data, uint16_t len) {
  return netiface->writeSocket(handle, data, len);
}

extern "C" void NetInterface_close_socket(NetInterface* netiface, int handle) {
  netiface->closeSocket(handle);
}
