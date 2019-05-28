#include "TobMbedtlsLL.h"

#include <strings.h>

#include "MIAS.h"

static MIAS mias;
static SEInterface* iface = nullptr;
static char mias_pin[16];

struct tob_mbedtls_ctx {
  mias_key_pair_t* kp;
};

static tob_mbedtls_ctx contexts[TOB_MBEDTLS_MAX_KEYS];
static int num_contexts = 0;


static size_t tob_mbedtls_len(void* ctx) {
  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;

  return (tob_ctx->kp->size_in_bits / 8);
}

static int tob_mbedtls_sign(void* ctx, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng, int mode,
                            mbedtls_md_type_t md_alg, unsigned int hashlen, const unsigned char* hash,
                            unsigned char* sig) {
  (void)f_rng;
  (void)p_rng;
  (void)mode;

  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;

  uint8_t algorithm = RSA_WITH_PKCS1_PADDING;

  switch (md_alg) {
    case MBEDTLS_MD_SHA1:
      algorithm |= ALGO_SHA1;
      break;
    case MBEDTLS_MD_SHA224:
      algorithm |= ALGO_SHA224;
      break;
    case MBEDTLS_MD_SHA256:
      algorithm |= ALGO_SHA256;
      break;
    case MBEDTLS_MD_SHA384:
      algorithm |= ALGO_SHA384;
      break;
    default:
      // SHA512 is not supported at the moment, MD* and RIPEMD-160 are not going to be supported
      return MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
  }

  if (!mias.select(false)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias.verifyPin((uint8_t*)mias_pin, strlen(mias_pin))) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias.signInit(algorithm, tob_ctx->kp->kid)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  uint16_t signature_len;
  if (!mias.signFinal(hash, hashlen, sig, &signature_len)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  return 0;
}

static int tob_mbedtls_decrypt(void* ctx, int mode, size_t* olen, const unsigned char* input, unsigned char* output,
                               size_t output_max_len) {
  (void)output_max_len;
  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;

  if (!mias.select(false)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias.verifyPin((uint8_t*)mias_pin, strlen(mias_pin))) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias.decryptInit(ALGO_RSA_PKCS1_PADDING, tob_ctx->kp->kid)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  uint16_t decrypted_size;
  if (!mias.decryptFinal(input, tob_ctx->kp->size_in_bits / 8, output, &decrypted_size)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  return 0;
}


bool tob_mbedtls_init_ll(SEInterface* seiface, const char* pin) {
  iface        = seiface;
  num_contexts = 0;

  if (!iface || !iface->open()) {
    return false;
  }

  Applet::closeAllChannels(iface);

  mias.init(iface);

  strncpy(mias_pin, pin, 15);
  mias_pin[15] = 0;

  return true;
}

bool tob_mbedtls_signing_key_ll(mbedtls_pk_context* pk, uint8_t key_id) {
  if (!mias.select(false)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias.verifyPin((uint8_t*)mias_pin, strlen(mias_pin))) {
    return false;
  }

  if (num_contexts >= TOB_MBEDTLS_MAX_KEYS) {
    return false;
  }

  auto ctx = &contexts[num_contexts++];

  if (!mias.getKeyPairByContainerId(key_id, &ctx->kp)) {
    delete ctx;
    return false;
  }

  if (mbedtls_pk_setup_rsa_alt(pk, ctx, tob_mbedtls_decrypt, tob_mbedtls_sign, tob_mbedtls_len) != 0) {
    delete ctx;
    return false;
  }
  return true;
}
