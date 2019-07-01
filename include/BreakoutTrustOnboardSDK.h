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

#define PEM_BUFFER_SIZE 10 * 1024
#define DER_BUFFER_SIZE 2 * 1024

#define TOB_MD_SHA1   0x10
#define TOB_MD_SHA224 0x30
#define TOB_MD_SHA256 0x40
#define TOB_MD_SHA384 0x50

#define TOB_ALGO_RSA_ISO9796_2 0x01
#define TOB_ALGO_RSA_PKCS1     0x02
#define TOB_ALGO_RSA_RFC_2409  0x03

typedef enum {
  TOB_ALGO_SHA1_RSA_PKCS1       = TOB_ALGO_RSA_PKCS1 | TOB_MD_SHA1,
  TOB_ALGO_SHA224_RSA_PKCS1     = TOB_ALGO_RSA_PKCS1 | TOB_MD_SHA224,
  TOB_ALGO_SHA256_RSA_PKCS1     = TOB_ALGO_RSA_PKCS1 | TOB_MD_SHA256,
  TOB_ALGO_SHA384_RSA_PKCS1     = TOB_ALGO_RSA_PKCS1 | TOB_MD_SHA384,
  TOB_ALGO_SHA1_RSA_ISO9796_2   = TOB_ALGO_RSA_ISO9796_2 | TOB_MD_SHA1,
  TOB_ALGO_SHA224_RSA_ISO9796_2 = TOB_ALGO_RSA_ISO9796_2 | TOB_MD_SHA224,
  TOB_ALGO_SHA256_RSA_ISO9796_2 = TOB_ALGO_RSA_ISO9796_2 | TOB_MD_SHA256,
  TOB_ALGO_SHA384_RSA_ISO9796_2 = TOB_ALGO_RSA_ISO9796_2 | TOB_MD_SHA384,
  TOB_ALGO_SHA1_RSA_RFC_2409    = TOB_ALGO_RSA_RFC_2409 | TOB_MD_SHA1,
  TOB_ALGO_SHA224_RSA_RFC_2409  = TOB_ALGO_RSA_RFC_2409 | TOB_MD_SHA224,
  TOB_ALGO_SHA256_RSA_RFC_2409  = TOB_ALGO_RSA_RFC_2409 | TOB_MD_SHA256,
  TOB_ALGO_SHA384_RSA_RFC_2409  = TOB_ALGO_RSA_RFC_2409 | TOB_MD_SHA384,
} tob_algorithm_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize Trust Onboard connection to cellular module with specified device.
 * Device must not be in use by another application in the system and must be
 * accessible as the current user.
 * @param device - full path to cellular module UART or "pcsc:N" for PC/SC device
 * @param baudrate - baud rate for a serial UART, ignored for PC/SC
 * @return 0 if successful, -1 if initialization fails
 */
extern int tobInitialize(const char *device, int baudrate);

/**
 * Extract the Available public certificate in PEM form.  The certificate
 * buffer will contain the device certificate itself and its preceding
 * intemediate certificates.
 * @param cert - buffer to receive the PEM certificate chain - should be allocated with at least size of
 * PEM_BUFFER_SIZE
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
 * @param pk - buffer to receive the DER private key - should be allocated with at least size of DER_BUFFER_SIZE
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
 * @param pk - buffer to receive the PEM private key - should be allocated with at least size of PEM_BUFFER_SIZE
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
 * @param cert - buffer to receive the PEM certificate chain - should be allocated with at least size of PEM_BUFFER_SIZE
 * @param cert_size - pointer to int that will receive the true number of bytes for the PEM certificate chain
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_INVALID_NAME_ERROR
 *   ERR_SE_EF_READ_OBJECT_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 */
extern int tobExtractSigningCertificate(uint8_t *cert, int *cert_size, const char *pin);

/**
 * Sign a message digest with a signing key
 * @param algorithm - signing algoritm and digest to use
 * @param hash - digest to sign
 * @param hash_len - length of the digest in bytes
 * @param signature - signature output buffer (allocate a large enough in advance!)
 * @param signature_len - length of the signature in bytes
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 */
extern int tobSigningSign(tob_algorithm_t algorithm, const uint8_t *hash, int hash_len, uint8_t* signature, int* signature_len, const char* pin);

/**
 * Decrypt data with a signing key
 * @param cipher - data to decrypt
 * @param cipher_len - length of the data
 * @param plain - buffer for the decrypted data (allocate a large enough in advance!)
 * @param plain_len - length of the decrypted data in bytes
 * @param pin - PIN1 for access to certificate.  Must be correct or SIM may be locked after repeated attempts.
 * @return 0 if successful, otherwise one of the following error codes:
 *   ERR_SE_BAD_KEY_NAME_ERROR
 *   ERR_SE_EF_VERIFY_PIN_ERROR
 */

extern int tobSigningDecrypt(const uint8_t *cipher, int cipher_len, uint8_t* plain, int* plain_len, const char* pin);

#ifdef __cplusplus
}
#endif

#endif /* __BREAKOUT_TOB_H__ */
