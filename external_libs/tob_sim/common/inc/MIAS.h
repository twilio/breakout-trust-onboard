/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#ifndef __MIAS_H__
#define __MIAS_H__

#include "Applet.h"

/*** HASH ALGORITHM **********************************************************/

#define ALGO_SHA1 0x10
#define ALGO_SHA224 0x30
#define ALGO_SHA256 0x40
#define ALGO_SHA384 0x50
#define ALGO_SHA512 0x60

/*** SIGN ALGORITHM **********************************************************/

#define RSA_WITH_ISO9796_2_PADDING 0x01
#define RSA_WITH_PKCS1_PADDING 0x02
#define RSA_WITH_RFC_2409_PADDING 0x03
#define ECDSA 0x04

#define ALGO_SHA1_WITH_RSA_ISO9796_2_PADDING (ALGO_SHA1 | RSA_WITH_ISO9796_2_PADDING)
#define ALGO_SHA224_WITH_RSA_ISO9796_2_PADDING (ALGO_SHA224 | RSA_WITH_ISO9796_2_PADDING)
#define ALGO_SHA256_WITH_RSA_ISO9796_2_PADDING (ALGO_SHA256 | RSA_WITH_ISO9796_2_PADDING)
#define ALGO_SHA384_WITH_RSA_ISO9796_2_PADDING (ALGO_SHA384 | RSA_WITH_ISO9796_2_PADDING)
#define ALGO_SHA512_WITH_RSA_ISO9796_2_PADDING (ALGO_SHA512 | RSA_WITH_ISO9796_2_PADDING)

#define ALGO_SHA1_WITH_RSA_PKCS1_PADDING (ALGO_SHA1 | RSA_WITH_PKCS1_PADDING)
#define ALGO_SHA224_WITH_RSA_PKCS1_PADDING (ALGO_SHA224 | RSA_WITH_PKCS1_PADDING)
#define ALGO_SHA256_WITH_RSA_PKCS1_PADDING (ALGO_SHA256 | RSA_WITH_PKCS1_PADDING)
#define ALGO_SHA384_WITH_RSA_PKCS1_PADDING (ALGO_SHA384 | RSA_WITH_PKCS1_PADDING)
#define ALGO_SHA512_WITH_RSA_PKCS1_PADDING (ALGO_SHA512 | RSA_WITH_PKCS1_PADDING)

#define ALGO_SHA1_WITH_RSA_RFC_2409_PADDING (ALGO_SHA1 | RSA_WITH_RFC_2409_PADDING)
#define ALGO_SHA224_WITH_RSA_RFC_2409_PADDING (ALGO_SHA224 | RSA_WITH_RFC_2409_PADDING)
#define ALGO_SHA256_WITH_RSA_RFC_2409_PADDING (ALGO_SHA256 | RSA_WITH_RFC_2409_PADDING)
#define ALGO_SHA384_WITH_RSA_RFC_2409_PADDING (ALGO_SHA384 | RSA_WITH_RFC_2409_PADDING)
#define ALGO_SHA512_WITH_RSA_RFC_2409_PADDING (ALGO_SHA512 | RSA_WITH_RFC_2409_PADDING)

#define ALGO_SHA1_WITH_ECDSA (ALGO_SHA1 | ECDSA)
#define ALGO_SHA224_WITH_ECDSA (ALGO_SHA224 | ECDSA)
#define ALGO_SHA256_WITH_ECDSA (ALGO_SHA256 | ECDSA)
#define ALGO_SHA384_WITH_ECDSA (ALGO_SHA384 | ECDSA)
#define ALGO_SHA512_WITH_ECDSA (ALGO_SHA512 | ECDSA)

/*** CIPHER ALGORITHM ********************************************************/

#define ALGO_RSA_PKCS1_PADDING 0x1A

/*** KEY PAIR INFO ***********************************************************/

#define RSA_KEY_PAIR_FLAG (1 << 0)
#define ECC_KEY_PAIR_FLAG (1 << 1)
#define SIGNATURE_KEY_PAIR_FLAG (1 << 2)
#define DECRYPTION_KEY_PAIR_FLAG (1 << 3)

typedef struct mias_key_pair_s {
  uint8_t kid;
  uint16_t flags;
  uint16_t size_in_bits;

  uint8_t pub_file_id[2];
  bool has_cert;
} mias_key_pair_t;

typedef struct mias_file_s {
  uint16_t efid;
  uint8_t dir[9];
  uint8_t name[9];
  uint16_t size;
} mias_file_t;

#ifdef __cplusplus

class MIAS : public Applet {
 public:
  static constexpr int key_pool_size  = 16;
  static constexpr int file_pool_size = 32;
  // Create an instance of MIAS Applet.
  MIAS(void);
  ~MIAS(void);

  // Unlock applet features by verifying user's pin.
  // Returns true in case verify pin was successful, false otherwise.
  bool verifyPin(uint8_t* pin, uint16_t pinLen);

  // Change user's pin value used to unlock applet features.
  // Returns true in case change pin was successful, false otherwise.
  bool changePin(uint8_t* oldPin, uint16_t oldPinLen, uint8_t* newPin, uint16_t newPinLen);

  // Change user's pin value used to unlock applet features.
  // Returns true in case change pin was successful, false otherwise.
  bool bind(uint8_t* pin, uint16_t pinLen, uint8_t* name, uint16_t nameLen) {
    return changePin(pin, pinLen, name, nameLen);
  }

  // Get key pair stored on the container identify by the provided id.
  // Kp parameter is a pointer of the found key pair, NULL otherwise.
  // Returns true in case operation was successful, false otherwise.
  bool getKeyPairByContainerId(uint8_t container_id, mias_key_pair_t** kp);

  // Get certificate on the container identify by the provided id.
  // cert parameter is a buffer to contain the resulted certificate. If NULL only length is returned.
  // Returns true in case operation was successful, false otherwise.
  bool getCertificateByContainerId(uint8_t container_id, uint8_t* cert, uint16_t* certLen);

  // Get P11 object identify by the provided label.
  // object parameter is a buffer to contain the extracted object. If NULL, only length is returned.
  // Returns true in case operation was successful, false otherwise.
  bool p11GetObjectByLabel(uint8_t* label, uint16_t labelLen, uint8_t* object, uint16_t* objectLen);

  // Prepare context in applet prior computing a hash.
  // Algorithm parameter is the targetted hashing algorithm.
  // Returns true in case preparing context was successful, false otherwise.
  bool hashInit(uint8_t algorithm);

  // Send chunk of data to be hashed. May be called several times.
  // Returns true in case sending chunk of data was successful, false otherwise.
  bool hashUpdate(const uint8_t* data, uint16_t dataLen);

  // Compute hash on previously received data.
  // Hash parameter is a buffer which will contain the resulted hash.
  // Returns true in case hashing was successful, false otherwise.
  bool hashFinal(uint8_t* hash, uint16_t* hashLen);

  // Prepare context in applet prior computing a signature.
  // Algorithm parameter is the targetted signature algorithm.
  // Key parameter is the id of the targetted key to used within the applet.
  // Returns true in case preparing context was successful, false otherwise.
  bool signInit(uint8_t algorithm, uint8_t key);

  // Compute signature
  // Hash parameter is a buffer which contain data to encrypt using key to compute signature.
  // Signature parameter is a buffer which will contain the resulted signature.
  // Returns true in case signing was successful, false otherwise.
  bool signFinal(const uint8_t* hash, uint16_t hashLen, uint8_t* signature, uint16_t* signatureLen);

  // Prepare context in applet prior decrypting data.
  // Algorithm parameter is the targetted decrypt algorithm.
  // Key parameter is the id of the targetted key to used within the applet.
  // Returns true in case preparing context was successful, false otherwise.
  bool decryptInit(uint8_t algorithm, uint8_t key);

  // Decrypt provided data and returns the corresponding plain data.
  // Returns true in case decrypting was successful, false otherwise.
  bool decryptFinal(const uint8_t* data, uint16_t dataLen, uint8_t* plain, uint16_t* plainLen);


  // List existing key pairs.
  // Returns true in case operation was successful, false otherwise.
  bool listKeyPairs(void);

 private:
  mias_key_pair_t _keypairs[key_pool_size];
  int _keypairs_num;

  mias_file_t _files[file_pool_size];
  int _files_num;

  uint8_t _hashAlgo;

  uint8_t _signAlgo;
  uint8_t _signKey;

  uint8_t _decryptAlgo;
  uint8_t _decryptKey;


  bool mseSetBeforeHash(uint8_t algorithm);
  bool psoHashInternally(uint8_t algorithm, const uint8_t* data, uint16_t dataLen);
  bool psoHashInternallyFinal(uint8_t* hash, uint16_t* hashLen);
  bool psoHashExternally(uint8_t algorithm, const uint8_t* hash, uint16_t hashLen);

  bool mseSetBeforeSignature(uint8_t algorithm, uint8_t key);
  bool psoComputeDigitalSignature(uint8_t* signature, uint16_t* signatureLen);

  bool mseSetBeforeDecrypt(uint8_t algorithm, uint8_t key);
  bool psoDecipher(const uint8_t* data, uint16_t dataLen, uint8_t* plain, uint16_t* plainLen);
};

#else

typedef struct MIAS MIAS;

MIAS* MIAS_create(void);
void MIAS_destroy(MIAS* mias);

bool MIAS_verify_pin(MIAS* mias, uint8_t* pin, uint16_t pin_len);
bool MIAS_change_pin(MIAS* mias, uint8_t* old_pin, uint16_t old_pin_len, uint8_t* new_pin, uint16_t new_pin_len);

bool MIAS_get_key_pair_by_container_id(MIAS* mias, uint8_t container_id, mias_key_pair_t** kp);
bool MIAS_get_certificate_by_container_id(MIAS* mias, uint8_t container_id, uint8_t** cert, uint16_t* cert_len);
bool MIAS_p11_get_object_by_label(MIAS* mias, uint8_t* label, uint16_t label_len, uint8_t** object,
                                  uint16_t* object_len);

bool MIAS_hash_init(MIAS* mias, uint8_t algorithm);
bool MIAS_hash_update(MIAS* mias, uint8_t* data, uint16_t data_len);
bool MIAS_hash_final(MIAS* mias, uint8_t* hash, uint16_t* hash_len);

bool MIAS_sign_init(MIAS* mias, uint8_t algorithm, uint8_t key);
bool MIAS_sign_final(MIAS* mias, uint8_t* hash, uint16_t hash_len, uint8_t* signature, uint16_t* signature_len);

bool MIAS_decrypt_init(MIAS* mias, uint8_t algorithm, uint8_t key);
bool MIAS_decrypt_final(MIAS* mias, uint8_t* data, uint16_t data_len, uint8_t* plain, uint16_t* plain_len);

#endif

#endif /* __MIAS_H__ */
