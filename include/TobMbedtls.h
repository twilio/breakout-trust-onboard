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

// Set MbedTLS signing key to Trust Onboard signing key
// @param pk - MbedTLS private key. Trust Onboard library should be initialized.
// @param pin - PIN code for MIAS applet
// @param signing - Whether to use signing (true) or available (false) key
// @return success status
bool tob_mbedtls_setup_key(mbedtls_pk_context* pk, const char* pin, bool signing);

#ifdef __cplusplus
}
#endif

#endif  // __TOB_MBEDTLS_H__
