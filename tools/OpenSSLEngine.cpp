#include <string.h>
#include <openssl/engine.h>
#include <openssl/pem.h>

#include "BreakoutTrustOnboardSDK.h"

static const char* engine_id   = "tob_mias";
static const char* engine_name = "Trust on Board MIAS engine";

static int ex_data_idx     = -1;
static int rsa_ex_data_idx = -1;

#define CMD_PIN ENGINE_CMD_BASE
#define CMD_MODEM_DEVICE (ENGINE_CMD_BASE + 1)
#define CMD_MODEM_BAUDRATE (ENGINE_CMD_BASE + 2)
#define CMD_LOAD_CERT_CTRL (ENGINE_CMD_BASE + 3)

static const ENGINE_CMD_DEFN engine_cmd_defns[] = {
    {CMD_PIN, "PIN", "Card's PIN code", ENGINE_CMD_FLAG_STRING},
    {CMD_MODEM_DEVICE, "MODEM_DEVICE",
     "Device, used to connect to Trust Onboard SIM. Either '/dev/<serial_device>' or 'pcsc:N'", ENGINE_CMD_FLAG_STRING},
    {CMD_MODEM_BAUDRATE, "MODEM_BAUDRATE", "Baudrate for a serial interface", ENGINE_CMD_FLAG_NUMERIC},
    {CMD_LOAD_CERT_CTRL, "LOAD_CERT_CTRL", "Load public certificate from engine", ENGINE_CMD_FLAG_STRING},
    {0, NULL, NULL, 0}};

struct tob_ctx_t {
  int initialized;
  char* pin;
  char* modem_device;
  int modem_baudrate;
#if OPENSSL_VERSION_NUMBER >= 0x10100004L
  CRYPTO_RWLOCK* sim_lock;
#else
  int sim_lock;
#endif
};

#if OPENSSL_VERSION_NUMBER >= 0x10100004L

static inline CRYPTO_RWLOCK* ssl_lock_universal_new() {
  return CRYPTO_THREAD_lock_new();
}

static inline void ssl_lock_universal_free(CRYPTO_RWLOCK* lock) {
  CRYPTO_THREAD_lock_free(lock);
}

static inline void ssl_lock_universal_lock(CRYPTO_RWLOCK* lock) {
  while (!CRYPTO_THREAD_write_lock(lock))
    ;
}

static inline void ssl_lock_universal_unlock(CRYPTO_RWLOCK* lock) {
  CRYPTO_THREAD_unlock(lock);
}

#else

static inline int ssl_lock_universal_new() {
  return CRYPTO_get_dynlock_create_callback() ? CRYPTO_get_new_dynlockid() : 0;
}

static inline void ssl_lock_universal_free(int lock) {
  CRYPTO_destroy_dynlockid(lock);
}

static inline void ssl_lock_universal_lock(int lock) {
  CRYPTO_w_lock(lock);
}

static inline void ssl_lock_universal_unlock(int lock) {
  CRYPTO_w_unlock(lock);
}

#endif

struct tob_key_t {
  tob_ctx_t* tob_ctx;
};

static int tob_set_ctx(ENGINE* e, tob_ctx_t** ctx) {
  tob_ctx_t* c = (tob_ctx_t*)calloc(1, sizeof(tob_ctx_t));
  int ret      = 1;

  if (c == NULL) {
    fprintf(stderr, "Failed to allocate context\n");
    return 0;
  }

  c->pin            = strdup("0000");
  c->modem_device   = strdup("/dev/ttyACM0");
  c->modem_baudrate = 115200;

  if ((*ctx = (tob_ctx_t*)ENGINE_get_ex_data(e, ex_data_idx)) == NULL) {
    ret = ENGINE_set_ex_data(e, ex_data_idx, c);
    if (ret) {
      *ctx = c;
    }
  }

  return ret;
}

static tob_ctx_t* tob_get_ctx(ENGINE* e) {
  tob_ctx_t* ctx;
  if (ex_data_idx < 0) {
    ex_data_idx = ENGINE_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    if (ex_data_idx < 0) {
      fprintf(stderr, "Failed to get ex data index\n");
      return 0;
    }
  }

  ctx = (tob_ctx_t*)ENGINE_get_ex_data(e, ex_data_idx);
  if ((ctx == NULL) && !tob_set_ctx(e, &ctx)) {
    return NULL;
  }

  return ctx;
}

static int tob_engine_init(ENGINE* engine) {
  tob_ctx_t* ctx = NULL;

  if (ex_data_idx < 0) {
  } else {
    ctx = (tob_ctx_t*)ENGINE_get_ex_data(engine, ex_data_idx);
  }

  if (ctx == NULL) {
    ctx = tob_get_ctx(engine);
    ENGINE_set_ex_data(engine, ex_data_idx, ctx);
  }

  if (!ctx->initialized) {
    if (tobInitialize(ctx->modem_device, ctx->modem_baudrate) != 0) {
      return 0;
    }

    ctx->sim_lock    = ssl_lock_universal_new();
    ctx->initialized = 1;
  }
  return 1;
}

static int tob_engine_finish(ENGINE* engine) {
  tob_ctx_t* ctx = tob_get_ctx(engine);

  if (ctx == NULL) {
    return 1;
  }

  // take ownership of the SIM before destroying applets
  if (ctx->sim_lock) {
    ssl_lock_universal_lock(ctx->sim_lock);
  }

  // TODO: deinitialization in ToB library

  if (ctx->pin) {
    free(ctx->pin);
  }

  if (ctx->modem_device) {
    free(ctx->modem_device);
  }

  if (ctx->sim_lock) {
    ssl_lock_universal_unlock(ctx->sim_lock);
    ssl_lock_universal_free(ctx->sim_lock);
  }

  free(ctx);

  ENGINE_set_ex_data(engine, ex_data_idx, NULL);
  return 1;
}

static X509* tob_extract_certificate(ENGINE* engine, bool signing);

static int tob_engine_ctrl(ENGINE* e, int cmd, long i, void* p, void (*f)(void)) {
  tob_ctx_t* ctx = tob_get_ctx(e);

  if (ctx == NULL) {
    fprintf(stderr, "No ToB context\n");
    return 0;
  }

  switch (cmd) {
    case CMD_PIN:
      if (ctx->pin != NULL) {
        free(ctx->pin);
      }
      ctx->pin = (p) ? strdup((const char*)p) : NULL;
      return 1;

    case CMD_MODEM_DEVICE:
      if (ctx->modem_device != NULL) {
        free(ctx->modem_device);
      }
      ctx->modem_device = strdup((char*)p);
      return 1;

    case CMD_MODEM_BAUDRATE:
      ctx->modem_baudrate = i;
      return 1;

    case CMD_LOAD_CERT_CTRL: {
      struct load_cert_params {
        const char* cert_id;
        X509* cert;
      }* params = (struct load_cert_params*)p;

      if (strcmp(params->cert_id, "signing") == 0) {
        params->cert = tob_extract_certificate(e, true);
      } else if (strcmp(params->cert_id, "available") == 0) {
        params->cert = tob_extract_certificate(e, false);
      } else {
        fprintf(stderr, "Invalid cert name: %s\n", params->cert_id);
        params->cert = NULL;
      }

      return (params->cert != NULL);
    }
  }
  fprintf(stderr, "Unknown command: %d\n", cmd);
  return 0;
}

static int tob_key_signing_sign(const unsigned char* m, unsigned int m_length, unsigned char* sigret,
                                unsigned int* siglen, tob_key_t* key_info) {
  if (key_info == NULL || key_info->tob_ctx == NULL || key_info->tob_ctx->pin == NULL) {
    fprintf(stderr, "TOB signing: invalid key_info\n");
    return 0;
  }

  tob_algorithm_t sig_algo;
  switch (m_length) {
    case 20:
      sig_algo = TOB_ALGO_SHA1_RSA_PKCS1;
      break;
    case 32:
      sig_algo = TOB_ALGO_SHA256_RSA_PKCS1;
      break;
    case 48:
      sig_algo = TOB_ALGO_SHA384_RSA_PKCS1;
      break;
    default:
      ssl_lock_universal_unlock(key_info->tob_ctx->sim_lock);
      return 0;
  }

  ssl_lock_universal_lock(key_info->tob_ctx->sim_lock);

  int sig_size;
  int ret = (tobSigningSign(sig_algo, m, m_length, sigret, &sig_size, key_info->tob_ctx->pin) == 0) && (sig_size >= 0);

  if (ret) {
    *siglen = sig_size;
  }

  ssl_lock_universal_unlock(key_info->tob_ctx->sim_lock);
  return ret;
}

static int tob_key_signing_decrypt(int flen, const unsigned char* from, unsigned char* to, tob_key_t* key_info) {
  if (key_info == NULL || key_info->tob_ctx == NULL || key_info->tob_ctx->pin == NULL) {
    fprintf(stderr, "TOB decryption: invalid key_info\n");
    return 0;
  }

  ssl_lock_universal_lock(key_info->tob_ctx->sim_lock);

  int to_size;
  int ret = (tobSigningDecrypt(from, flen, to, &to_size, key_info->tob_ctx->pin) == 0);

  ssl_lock_universal_unlock(key_info->tob_ctx->sim_lock);
  return ret;
}

// Sign with a private key
static int tob_engine_signing_rsa_sign(int type, const unsigned char* m, unsigned int m_length, unsigned char* sigret,
                                       unsigned int* siglen, const RSA* rsa) {
  tob_key_t* key_info = (tob_key_t*)RSA_get_ex_data(rsa, rsa_ex_data_idx);
  return tob_key_signing_sign(m, m_length, sigret, siglen, key_info);
}

// Decrypt with a private key
static int tob_engine_signing_rsa_decrypt(int flen, const unsigned char* from, unsigned char* to, RSA* rsa,
                                          int padding) {
  if (padding != RSA_PKCS1_PADDING) {
    fprintf(stderr, "Only PKCS1 padding is supported in %s\n", engine_id);
  }

  tob_key_t* key_info = (tob_key_t*)RSA_get_ex_data(rsa, rsa_ex_data_idx);
  return tob_key_signing_decrypt(flen, from, to, key_info);
}

static int tob_engine_signing_rsa_finish(RSA* rsa) {
  if (rsa_ex_data_idx < 0) {
    return 1;
  }

  void* priv = RSA_get_ex_data(rsa, rsa_ex_data_idx);
  free(priv);

  RSA_set_ex_data(rsa, rsa_ex_data_idx, NULL);

  return 1;
}

static RSA_METHOD* tob_engine_signing_rsa(void) {
  static RSA_METHOD* tob_engine_meth = NULL;

  if (tob_engine_meth == NULL) {
#if OPENSSL_VERSION_NUMBER >= 0x10100005L
    tob_engine_meth = RSA_meth_dup(RSA_get_default_method());
    RSA_meth_set1_name(tob_engine_meth, "Trust Onboard RSA method");
    RSA_meth_set_flags(tob_engine_meth, 0);
    RSA_meth_set_sign(tob_engine_meth, tob_engine_signing_rsa_sign);
    RSA_meth_set_priv_dec(tob_engine_meth, tob_engine_signing_rsa_decrypt);
    RSA_meth_set_finish(tob_engine_meth, tob_engine_signing_rsa_finish);
#else
    static const RSA_METHOD* default_method = RSA_get_default_method();
    tob_engine_meth = (RSA_METHOD*)OPENSSL_malloc(sizeof(RSA_METHOD));
    if (tob_engine_meth == NULL) return NULL;
    memcpy(tob_engine_meth, default_method, sizeof(RSA_METHOD));

    tob_engine_meth->name = OPENSSL_strdup("Trust Onboard RSA method");
    tob_engine_meth->flags = RSA_FLAG_SIGN_VER;
    tob_engine_meth->rsa_sign = tob_engine_rsa_sign;
    tob_engine_meth->rsa_priv_dec = tob_engine_rsa_decrypt;
    tob_engine_meth->finish = tob_engine_rsa_finish;
#endif
  }

  return tob_engine_meth;
}

static X509* tob_extract_certificate(ENGINE* engine, bool signing) {
  tob_ctx_t* ctx = tob_get_ctx(engine);
  if (ctx == NULL) {
    return NULL;
  }

  X509* cert_x509 = NULL;

  if (signing) {
    int cert_len;

    ssl_lock_universal_lock(ctx->sim_lock);
    if (tobExtractSigningCertificate(NULL, NULL, NULL, &cert_len, ctx->pin) != 0) {
      fprintf(stderr, "Couldn't get signing certificate length\n");
      ssl_lock_universal_unlock(ctx->sim_lock);
      return NULL;
    }

    uint8_t* cert = (uint8_t*)malloc(cert_len * sizeof(uint8_t));
    if (!cert) {
      fprintf(stderr, "Couldn't allocate memory for signing certificate\n");
      ssl_lock_universal_unlock(ctx->sim_lock);
      return NULL;
    }

    if (tobExtractSigningCertificate(NULL, NULL, cert, &cert_len, ctx->pin) != 0) {
      fprintf(stderr, "Couldn't get signing certificate\n");
      ssl_lock_universal_unlock(ctx->sim_lock);
      return NULL;
    }

    ssl_lock_universal_unlock(ctx->sim_lock);
    BIO* cert_bio = BIO_new_mem_buf(cert, cert_len);
    if (!cert_bio) {
      fprintf(stderr, "Couldn't create BIO for signing certificate\n");
      free(cert);
      return NULL;
    }

    cert_x509 = d2i_X509_bio(cert_bio, NULL);
    BIO_free(cert_bio);
    free(cert);
  } else {
    int cert_len;

    ssl_lock_universal_lock(ctx->sim_lock);
    if (tobExtractAvailableCertificate(NULL, &cert_len, ctx->pin) != 0) {
      fprintf(stderr, "Couldn't get available certificate length\n");
      ssl_lock_universal_unlock(ctx->sim_lock);
      return NULL;
    }

    uint8_t* cert = (uint8_t*)malloc(cert_len * sizeof(uint8_t));
    if (!cert) {
      fprintf(stderr, "Couldn't allocate memory for available certificate\n");
      ssl_lock_universal_unlock(ctx->sim_lock);
      return NULL;
    }

    if (tobExtractAvailableCertificate(cert, &cert_len, ctx->pin) != 0) {
      fprintf(stderr, "Couldn't get available certificate\n");
      ssl_lock_universal_unlock(ctx->sim_lock);
      return NULL;
    }

    ssl_lock_universal_unlock(ctx->sim_lock);
    BIO* cert_bio = BIO_new_mem_buf(cert, cert_len);
    if (!cert_bio) {
      fprintf(stderr, "Couldn't create BIO for available certificate\n");
      free(cert);
      return NULL;
    }

    cert_x509 = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
    BIO_free(cert_bio);
    free(cert);
  }

  return cert_x509;
}

static EVP_PKEY* tob_engine_load_pubkey(ENGINE* engine, bool signing) {
  X509* cert_x509 = tob_extract_certificate(engine, signing);
  if (cert_x509 == NULL) {
    fprintf(stderr, "Couldn't parse certificate\n");
    return NULL;
  }

  EVP_PKEY* res = X509_get_pubkey(cert_x509);

  X509_free(cert_x509);

  return res;
}

static EVP_PKEY* tob_engine_load_pubkey_function(ENGINE* engine, const char* name, UI_METHOD* ui_method,
                                                 void* callback_data) {
  (void)ui_method;
  (void)callback_data;

  if (strcmp(name, "signing") == 0) {
    return tob_engine_load_pubkey(engine, true);
  } else if (strcmp(name, "available") == 0) {
    return tob_engine_load_pubkey(engine, false);
  } else {
    fprintf(stderr, "Invalid key name: %s\n", name);
    return NULL;
  }
}

static EVP_PKEY* tob_engine_load_signing_privkey(ENGINE* engine) {
  tob_ctx_t* ctx = tob_get_ctx(engine);
  if (ctx == NULL) {
    return NULL;
  }

  EVP_PKEY* pubkey = tob_engine_load_pubkey(engine, true);

  if (!pubkey) {
    fprintf(stderr, "Couldn't load signing public key\n");
    return NULL;
  }

#if OPENSSL_VERSION_NUMBER >= 0x10100003L
  RSA* pubkey_rsa = EVP_PKEY_get0_RSA(pubkey);
#else
  RSA* pubkey_rsa = pubkey->pkey.rsa;
#endif

  if (!pubkey_rsa) {
    EVP_PKEY_free(pubkey);
    return NULL;
  }

  RSA* rsa_key = RSA_new();
  if (!rsa_key) {
    EVP_PKEY_free(pubkey);
    return NULL;
  }

#if OPENSSL_VERSION_NUMBER >= 0x10100003L
  const BIGNUM* pub_n;
  const BIGNUM* pub_e;
  RSA_get0_key(pubkey_rsa, &pub_n, &pub_e, NULL);
  RSA_set0_key(rsa_key, BN_dup(pub_n), BN_dup(pub_e), NULL);
#else
  rsa_key->e = BN_dup(pubkey_rsa->e);
  rsa_key->n = BN_dup(pubkey_rsa->n);
#endif

  EVP_PKEY_free(pubkey);

  if (rsa_ex_data_idx < 0) {
    rsa_ex_data_idx = RSA_get_ex_new_index(0, NULL, NULL, NULL, NULL);

    if (rsa_ex_data_idx < 0) {
      return NULL;
    }
  }

  tob_key_t* key_private = (tob_key_t*)malloc(sizeof(tob_key_t));
  key_private->tob_ctx   = ctx;
  RSA_set_ex_data(rsa_key, rsa_ex_data_idx, key_private);

  RSA_set_method(rsa_key, tob_engine_signing_rsa());

  EVP_PKEY* res = EVP_PKEY_new();

  if (!res) {
    RSA_free(rsa_key);
    return NULL;
  }


  EVP_PKEY_set1_RSA(res, rsa_key);
  RSA_free(rsa_key);
  return res;
}

static EVP_PKEY* tob_engine_load_available_privkey(ENGINE* engine) {
  tob_ctx_t* ctx = tob_get_ctx(engine);
  if (ctx == NULL) {
    return NULL;
  }

  int key_len;
  ssl_lock_universal_lock(ctx->sim_lock);
  if (tobExtractAvailablePrivateKey(NULL, NULL, NULL, &key_len, ctx->pin) != 0) {
    fprintf(stderr, "Couldn't get available key length\n");
    ssl_lock_universal_unlock(ctx->sim_lock);
    return NULL;
  }

  uint8_t* key = (uint8_t*)malloc(key_len * sizeof(uint8_t));
  if (!key) {
    fprintf(stderr, "Couldn't allocate memory for available private key\n");
    ssl_lock_universal_unlock(ctx->sim_lock);
    return NULL;
  }

  if (tobExtractAvailablePrivateKey(NULL, NULL, key, &key_len, ctx->pin) != 0) {
    fprintf(stderr, "Couldn't get available key\n");
    ssl_lock_universal_unlock(ctx->sim_lock);
    return NULL;
  }
  ssl_lock_universal_unlock(ctx->sim_lock);

  BIO* key_bio = BIO_new_mem_buf(key, key_len);
  if (!key_bio) {
    fprintf(stderr, "Couldn't create BIO for available private key\n");
    free(key);
    return NULL;
  }

  EVP_PKEY* res = d2i_PrivateKey_bio(key_bio, NULL);
  BIO_free(key_bio);
  free(key);

  return res;
}

static EVP_PKEY* tob_engine_load_privkey_function(ENGINE* engine, const char* name, UI_METHOD* ui_method,
                                                  void* callback_data) {
  (void)ui_method;
  (void)callback_data;

  if (strcmp(name, "signing") == 0) {
    return tob_engine_load_signing_privkey(engine);
  } else if (strcmp(name, "available") == 0) {
    return tob_engine_load_available_privkey(engine);
  } else {
    fprintf(stderr, "Invalid key name: %s\n", name);
    return NULL;
  }
}

extern "C" {
static int bind(ENGINE* e, const char* id);
}

static int bind(ENGINE* e, const char* id) {
  int ret = 0;

  if (!id || (strcmp(id, engine_id) == 0)) {
    if (!ENGINE_set_id(e, engine_id)) {
      fprintf(stderr, "ENGINE_set_id failed\n");
      goto err;
    }
  } else {
    fprintf(stderr, "Engine ID %s is not supported\n", id);
    return 0;
  }


  if (!ENGINE_set_name(e, engine_name)) {
    fprintf(stderr, "ENGINE_set_name failed\n");
    goto err;
  }

  if (!ENGINE_set_cmd_defns(e, engine_cmd_defns)) {
    fprintf(stderr, "ENGINE_set_cmd_defns failed\n");
    goto err;
  }

  if (!ENGINE_set_init_function(e, tob_engine_init)) {
    fprintf(stderr, "ENGINE_set_init_function failed\n");
    goto err;
  }

  if (!ENGINE_set_finish_function(e, tob_engine_finish)) {
    fprintf(stderr, "ENGINE_set_finish_function failed\n");
    goto err;
  }

  if (!ENGINE_set_ctrl_function(e, tob_engine_ctrl)) {
    fprintf(stderr, "ENGINE_set_ctrl_function failed\n");
    goto err;
  }

  if (!ENGINE_set_load_pubkey_function(e, tob_engine_load_pubkey_function)) {
    fprintf(stderr, "ENGINE_set_load_pubkey_function failed\n");
    goto err;
  }

  if (!ENGINE_set_load_privkey_function(e, tob_engine_load_privkey_function)) {
    fprintf(stderr, "ENGINE_set_load_privkey_function failed\n");
    goto err;
  }

  return 1;
err:
  ERR_print_errors_fp(stderr);
  return ret;
}

extern "C" {
IMPLEMENT_DYNAMIC_BIND_FN(bind)
IMPLEMENT_DYNAMIC_CHECK_FN()
}
