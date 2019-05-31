/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __TOB_MBEDTLS_LL_H__
#define __TOB_MBEDTLS_LL_H__

#include <mbedtls/ssl.h>
#include "SEInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TOB_MBEDTLS_MAX_KEYS 4

// Initialize Trust Onboard
// @param iface - serial interface to use
// @param pin - PIN code for MIAS applet
// @return success status
bool tob_mbedtls_init_ll(SEInterface* iface, const char* pin);

// Set MbedTLS signing key to Trust Onboard signing key
// @param pk - MbedTLS private key. Should be pre-initialized with mbedtls_pk_init
// @param key_id - Container ID to use. Normally 0.
// @return success status
bool tob_mbedtls_signing_key_ll(mbedtls_pk_context* pk, uint8_t key_id);

#ifdef __cplusplus
}
#endif

#endif  // __TOB_MBEDTLS_LL_H__
