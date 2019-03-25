/*
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
