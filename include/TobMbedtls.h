/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __TOB_MBEDTLS_H__
#define __TOB_MBEDTLS_H__

#include <mbedtls/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Trust Onboard
// @param iface_name - serial interface to use. Either of form "/dev/myserialdevice" or "pcsc:X"
// @param baudrate - baudrate for the serial interface. Ignored for PC/SC devices
// @param pin - PIN code for MIAS applet
// @return success status
bool tob_mbedtls_init(const char* iface_name, int baudrate, const char* pin);

// Set MbedTLS signing key to Trust Onboard signing key
// @param pk - MbedTLS private key. Should be pre-initialized with mbedtls_pk_init
// @param key_id - Container ID to use. Normally 0.
// @return success status
bool tob_mbedtls_signing_key(mbedtls_pk_context* pk, uint8_t key_id);

#ifdef __cplusplus
}
#endif

#endif  // __TOB_MBEDTLS_H__
