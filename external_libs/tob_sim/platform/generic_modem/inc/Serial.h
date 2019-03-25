/*
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdint.h>

class Serial {
 public:
  Serial(void);
  virtual ~Serial(void);

  virtual bool start(void)                                                             = 0;
  virtual bool send(char* data, unsigned long int toWrite, unsigned long int* written) = 0;
  virtual bool recv(char* data, unsigned long int toRead, unsigned long int* read)     = 0;
  virtual bool stop(void)                                                              = 0;
};

#endif /* __SERIAL_H__ */
