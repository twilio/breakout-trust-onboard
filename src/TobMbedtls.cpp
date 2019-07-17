#include "TobMbedtls.h"
#include "BreakoutTrustOnboardSDK.h"
#include <stdlib.h>
#include <strings.h>

#ifndef TOB_MBEDTLS_MAX_KEYS
#define TOB_MBEDTLS_MAX_KEYS 16
#endif

struct tob_mbedtls_ctx {
  char pin[16];
  size_t len;
};

static tob_mbedtls_ctx contexts[TOB_MBEDTLS_MAX_KEYS];
static int num_contexts = 0;

static size_t tob_mbedtls_signing_len(void* ctx) {
  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;
  return tob_ctx->len;
}

static int tob_mbedtls_signing_sign(void* ctx, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng, int mode,
                                    mbedtls_md_type_t md_alg, unsigned int hashlen, const unsigned char* hash,
                                    unsigned char* sig) {
  (void)f_rng;
  (void)p_rng;
  (void)mode;

  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;

  int algorithm = TOB_ALGO_RSA_PKCS1;

  switch (md_alg) {
    case MBEDTLS_MD_SHA1:
      algorithm |= TOB_MD_SHA1;
      break;
    case MBEDTLS_MD_SHA224:
      algorithm |= TOB_MD_SHA224;
      break;
    case MBEDTLS_MD_SHA256:
      algorithm |= TOB_MD_SHA256;
      break;
    case MBEDTLS_MD_SHA384:
      algorithm |= TOB_MD_SHA384;
      break;
    default:
      // SHA512 is not supported at the moment, MD* and RIPEMD-160 are not going to be supported
      return MBEDTLS_ERR_PK_FEATURE_UNAVAILABLE;
  }

  int signature_len;
  if (tobSigningSign((tob_algorithm_t)algorithm, hash, hashlen, sig, &signature_len, tob_ctx->pin) != 0) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  return 0;
}

static int tob_mbedtls_signing_decrypt(void* ctx, int mode, size_t* olen, const unsigned char* input,
                                       unsigned char* output, size_t output_max_len) {
  (void)mode;
  (void)output_max_len;

  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;

  int plain_len;
  if (tobSigningDecrypt(input, tob_ctx->len, output, &plain_len, tob_ctx->pin) != 0 || plain_len < 0) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  *olen = plain_len;

  return 0;
}

bool tob_mbedtls_setup_key(mbedtls_pk_context* pk, const char* pin, bool signing) {
  if (signing) {
    if (num_contexts >= TOB_MBEDTLS_MAX_KEYS) {
      return false;
    }

    auto ctx = &contexts[num_contexts++];

    strncpy(ctx->pin, pin, 15);
    ctx->pin[15] = 0;
    ctx->len     = tobSigningLen(pin);

    if (mbedtls_pk_setup_rsa_alt(pk, ctx, tob_mbedtls_signing_decrypt, tob_mbedtls_signing_sign,
                                 tob_mbedtls_signing_len) != 0) {
      delete ctx;
      return false;
    }
  } else {
    static unsigned char key_buf[DER_BUFFER_SIZE];
    int key_size;

    if (tobExtractAvailablePrivateKey(key_buf, &key_size, pin) != 0) {
      return false;
    }

    if (mbedtls_pk_parse_key(pk, key_buf, key_size, nullptr, 0) != 0) {
      return false;
    }
  }

  return true;
}
