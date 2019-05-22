#include "MbedtlsAsyncPrivate.h"
#include <stdlib.h>

#include "MIAS.h"
#include "GenericModem.h"

#ifdef PCSC_SUPPORT
#include "Pcsc.h"
#endif

static MIAS* mias               = nullptr;
static SEInterface* iface       = nullptr;
static mias_key_pair_t* keypair = nullptr;
static char* mias_pin           = nullptr;

struct tob_mbedtls_ctx {
  mias_key_pair_t* kp;
};

bool tob_mbedtls_init(const char* iface_path, const char* pin) {
  if (iface_path == nullptr) {
    return false;
  }

  if (strncmp(iface_path, "pcsc:", 5) == 0) {
#ifdef PCSC_SUPPORT
    long idx = strtol(iface_path + 5, 0, 10);  // device is al least 5 characters long, indexing is safe
    iface    = new PcscSEInterface((int)idx);
#else
    fprintf(stderr, "No pcsc support, please rebuild with -DPCSC_SUPPORT=ON\n");
    return false;
#endif
  } else {
    iface = new GenericModem(iface_path);
  }

  if (!iface->open()) {
    fprintf(stderr, "Can't open modem device\n");
    return false;
  }

  Applet::closeAllChannels(iface);

  mias = new MIAS();
  mias->init(iface);

  mias_pin = strdup(pin);

  return true;
}

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

  if (!mias->select(false)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias->verifyPin((uint8_t*)mias_pin, strlen(mias_pin))) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias->signInit(algorithm, tob_ctx->kp->kid)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  uint16_t signature_len;
  if (!mias->signFinal(hash, hashlen, sig, &signature_len)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  return 0;
}

static int tob_mbedtls_decrypt(void* ctx, int mode, size_t* olen, const unsigned char* input, unsigned char* output,
                               size_t output_max_len) {
  (void)output_max_len;
  tob_mbedtls_ctx* tob_ctx = (tob_mbedtls_ctx*)ctx;

  if (!mias->select(false)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias->verifyPin((uint8_t*)mias_pin, strlen(mias_pin))) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias->decryptInit(ALGO_RSA_PKCS1_PADDING, tob_ctx->kp->kid)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  uint16_t decrypted_size;
  if (!mias->decryptFinal(input, tob_ctx->kp->size_in_bits / 8, output, &decrypted_size)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  return 0;
}

bool tob_mbedtls_signing_key(mbedtls_pk_context* pk, uint8_t key_id) {
  if (!mias->select(false)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  if (!mias->verifyPin((uint8_t*)mias_pin, strlen(mias_pin))) {
    return false;
  }

  auto ctx = new tob_mbedtls_ctx;

  if (!mias->getKeyPairByContainerId(key_id, &ctx->kp)) {
    delete ctx;
    return false;
  }

  if (mbedtls_pk_setup_rsa_alt(pk, ctx, tob_mbedtls_decrypt, tob_mbedtls_sign, tob_mbedtls_len) != 0) {
    delete ctx;
    return false;
  }
  return true;
}
