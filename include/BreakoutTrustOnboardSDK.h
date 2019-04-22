/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __BREAKOUT_TOB_H__
#define __BREAKOUT_TOB_H__

#include <stdlib.h>
#include <stdint.h>

#define ERR_SE_MIAS_READ_OBJECT_ERROR -0x5400 /**< Mobile IAS read object failed. */
#define ERR_SE_EF_VERIFY_PIN_ERROR -0x5480    /**< Verifying pin to access EF failed. */
#define ERR_SE_EF_INVALID_NAME_ERROR -0x5500  /**< Trying to access an EF with an invalid name. */
#define ERR_SE_EF_READ_OBJECT_ERROR -0x5580   /**< EF read object failed. */
#define ERR_SE_BAD_KEY_NAME_ERROR -0x5600     /**< No matching key found with the given name. */

#define CERT_BUFFER_SIZE 10 * 1024
#define PK_BUFFER_SIZE 2 * 1024

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize Trust Onboard connection to cellular module with specified device.
 * Device must not be in use by another application in the system and must be
 * accessible as the current user.
 * @param device - full path to cellular module UART
 * @return 0 if successful, -1 if initialization fails
 */
extern int tobInitialize(const char *device);

/**
 * Extract the Available public certificate in PEM form.  The certificate
 * buffer will contain the device certificate itself and its preceding
 * intemediate certificates.
 * @param cert - buffer to receive the PEM certificate chain - should be allocated with at least size of
 * CERT_BUFFER_SIZE
 * @param cert_size - pointer to int that will receive the true number of bytes for the PEM certificate chain
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_INVALID_NAME_ERROR
 *   ERR_SE_EF_READ_OBJECT_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 */
extern int tobExtractAvailableCertificate(uint8_t *cert, int *cert_size, const char *pin);

/**
 * Extract the Available private key in DER form.
 *
 * NOTE: Care should be taken to secure the output of this function, the
 * private key should be kept secret.
 *
 * @param pk - buffer to receive the DER private key - should be allocated with at least size of PK_BUFFER_SIZE
 * @param pk_size - pointer to int that will receive the true number of bytes for the DER private key.
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_INVALID_NAME_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 *   ERR_SE_MIAS_READ_OBJECT_ERROR
 */
extern int tobExtractAvailablePrivateKey(uint8_t *pk, int *pk_size, const char *pin);

/**
 * Extract the Available private key in PEM form.
 *
 * NOTE: Care should be taken to secure the output of this function, the
 * private key should be kept secret.
 *
 * @param pk - buffer to receive the PEM private key - should be allocated with at least size of CERT_BUFFER_SIZE
 * @param pk_size - pointer to int that will receive the true number of bytes for the PEM private key.
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_INVALID_NAME_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 *   ERR_SE_MIAS_READ_OBJECT_ERROR
 */
extern int tobExtractAvailablePrivateKeyAsPem(uint8_t *pk, int *pk_size, const char *pin);

/**
 * Extract the Signing public certificate in PEM form.  The certificate
 * buffer will contain the device certificate itself and its preceding
 * intemediate certificates.
 * @param cert - buffer to receive the PEM certificate chain - should be allocated with at least size of CERT_BUFFER_SIZE
 * @param cert_size - pointer to int that will receive the true number of bytes for the PEM certificate chain
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_INVALID_NAME_ERROR
 *   ERR_SE_EF_READ_OBJECT_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 */
extern int tobExtractSigningCertificate(uint8_t *cert, int *cert_size, const char *pin);

#ifdef __cplusplus
}
#endif

#endif /* __BREAKOUT_TOB_H__ */
