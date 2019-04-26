#include <string.h>
#include <openssl/engine.h>

#include "MIAS.h"
#include "GenericModem.h"
#ifdef PCSC_SUPPORT
#include "Pcsc.h"
#endif

static const char* engine_id   = "tob_mias";
static const char* engine_name = "Trust on Board MIAS engine";

static int ex_data_idx     = -1;
static int rsa_ex_data_idx = -1;

#define CMD_PIN ENGINE_CMD_BASE
#define CMD_PCSC (ENGINE_CMD_BASE + 1)
#define CMD_MODEM_DEVICE (ENGINE_CMD_BASE + 2)
#define CMD_PCSC_IDX (ENGINE_CMD_BASE + 3)

static const ENGINE_CMD_DEFN engine_cmd_defns[] = {
    {CMD_PIN, "PIN", "Card's PIN code", ENGINE_CMD_FLAG_STRING},
    {CMD_PCSC, "PCSC", "Use PC/SC card reader intead of a serial modem", ENGINE_CMD_FLAG_NUMERIC},
    {CMD_MODEM_DEVICE, "MODEM_DEVICE", "If MODEM is specified, use this device as a serial interface",
     ENGINE_CMD_FLAG_STRING},
    {CMD_PCSC_IDX, "PCSC_IDX",
     "If MODEM is _not_ specified, use this card reader (better always specify as 0 and connect only one reader at a "
     "time)",
     ENGINE_CMD_FLAG_NUMERIC},
    {0, NULL, NULL, 0}};

struct tob_ctx_t {
  int initialized;
  char* pin;
  int is_pcsc;
  char* modem_device;
  int pcsc_idx;
  SEInterface* interface;
  MIAS* mias;
};

// Private data given to OpenSSL RSA structs
struct tob_key_t {
  // uint8_t mias_alg;
  uint8_t kid;
  // EVP_PKEY* evp_key;
  tob_ctx_t* tob_ctx;
};

/*
 * Based on eng_dyn
 */
static int tob_set_ctx(ENGINE* e, tob_ctx_t** ctx) {
  tob_ctx_t* c = (tob_ctx_t*)calloc(1, sizeof(tob_ctx_t));
  int ret      = 1;

  if (c == NULL) {
    fprintf(stderr, "Failed to allocate context\n");
    return 0;
  }

  c->pin          = strdup("0000");
  c->is_pcsc      = 0;
  c->modem_device = strdup("/dev/ttyACM0");
  c->pcsc_idx     = 0;

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
    if (!ctx->interface) {
      if (ctx->is_pcsc) {
#ifdef PCSC_SUPPORT
        ctx->interface = new PcscSEInterface(ctx->pcsc_idx);
#else
        return 0;
#endif
      } else {
        ctx->interface = new GenericModem(ctx->modem_device);
      }
    }

    if (!ctx->interface->open()) {
      return 0;
    }

    ctx->mias = new MIAS();
    ctx->mias->init(ctx->interface);
    ctx->initialized = 1;
  }
  return 1;
}

static int tob_engine_finish(ENGINE* engine) {
  tob_ctx_t* ctx = tob_get_ctx(engine);

  if (ctx == NULL) {
    return 0;
  }

  // TODO: do the clean-up
  return 0;
}

static int tob_engine_destroy(ENGINE* engine) {
  tob_ctx_t* ctx = tob_get_ctx(engine);

  if (ctx == NULL) {
    return 0;
  }

  // TODO: do the clean-up
  return 0;
}

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

    case CMD_PCSC:
      ctx->is_pcsc = i;
      return 1;

    case CMD_MODEM_DEVICE:
      if (ctx->modem_device != NULL) {
        free(ctx->modem_device);
      }
      ctx->modem_device = strdup((char*)p);
      return 1;

    case CMD_PCSC_IDX:
      ctx->pcsc_idx = i;
      return 1;
  }
  fprintf(stderr, "Unknown command: %d\n", cmd);
  return 0;
}

static int tob_key_sign(const unsigned char* m, unsigned int m_length, unsigned char* sigret, unsigned int* siglen,
                        tob_key_t* key_info) {
  if (key_info == NULL || key_info->tob_ctx == NULL || key_info->tob_ctx->mias == NULL) {
    fprintf(stderr, "TOB encryption: invalid key_info\n");
    return 0;
  }

  // TODO: some lock needs to be held here, also investigate MIAS multichannel capabilities
  if (!key_info->tob_ctx->mias->select(false)) {
    return 0;
  }

  if (!key_info->tob_ctx->mias->verifyPin((uint8_t*)key_info->tob_ctx->pin, strlen(key_info->tob_ctx->pin))) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  if (!key_info->tob_ctx->mias->signInit(ALGO_SHA512_WITH_RSA_PKCS1_PADDING, key_info->kid)) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  uint16_t sig_size;
  if (!key_info->tob_ctx->mias->signFinal(m, m_length, sigret, &sig_size)) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }
  *siglen = sig_size;

  key_info->tob_ctx->mias->deselect();
  return 1;
}

static int tob_key_encrypt(int flen, const unsigned char* from, unsigned char* to, tob_key_t* key_info,
                           int mias_padding) {
  if (key_info == NULL || key_info->tob_ctx == NULL || key_info->tob_ctx->mias == NULL) {
    fprintf(stderr, "TOB encryption: invalid key_info\n");
    return 0;
  }

  uint8_t mias_hash_alg;

  switch (flen) {
    case 20:
      mias_hash_alg = ALGO_SHA1;
      break;
    case 28:
      mias_hash_alg = ALGO_SHA224;
      break;
    case 32:
      mias_hash_alg = ALGO_SHA256;
      break;
    case 48:
      mias_hash_alg = ALGO_SHA384;
      break;
    case 64:
      mias_hash_alg = ALGO_SHA512;
      break;
    default:
      fprintf(stderr, "TOB encryption: invalid hash length: %d\n", flen);
      return 0;
  }

  // TODO: some lock needs to be held here, also investigate MIAS multichannel capabilities
  if (!key_info->tob_ctx->mias->select(false)) {
    return 0;
  }
  if (!key_info->tob_ctx->mias->signInit(mias_hash_alg | mias_padding, key_info->kid)) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  uint16_t sig_size;  // ignored, OpenSSL knows the expected size
  if (!key_info->tob_ctx->mias->signFinal(from, flen, to, &sig_size)) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  key_info->tob_ctx->mias->deselect();
  return 1;
}

static int tob_key_decrypt(int flen, const unsigned char* from, unsigned char* to, tob_key_t* key_info,
                           int mias_padding) {
  if (key_info == NULL || key_info->tob_ctx == NULL || key_info->tob_ctx->mias == NULL) {
    fprintf(stderr, "TOB decryption: invalid key_info\n");
    return 0;
  }
  // TODO: some lock needs to be held here, also investigate MIAS multichannel capabilities
  if (!key_info->tob_ctx->mias->select(false)) {
    return 0;
  }

  if (!key_info->tob_ctx->mias->verifyPin((uint8_t*)key_info->tob_ctx->pin, strlen(key_info->tob_ctx->pin))) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  if (!key_info->tob_ctx->mias->decryptInit(mias_padding, key_info->kid)) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  uint16_t plain_size;  // ignored, OpenSSL knows the expected size
  if (!key_info->tob_ctx->mias->decryptFinal(from, flen, to, &plain_size)) {
    key_info->tob_ctx->mias->deselect();
    return 0;
  }

  key_info->tob_ctx->mias->deselect();
  return 1;
}

// Sign with a private key
static int tob_engine_rsa_sign(int type, const unsigned char* m, unsigned int m_length, unsigned char* sigret,
                               unsigned int* siglen, const RSA* rsa) {
  tob_key_t* key_info = (tob_key_t*)RSA_get_ex_data(rsa, rsa_ex_data_idx);
  return tob_key_sign(m, m_length, sigret, siglen, key_info);
}

// Encrypt with a private key
static int tob_engine_rsa_encrypt(int flen, const unsigned char* from, unsigned char* to, RSA* rsa, int padding) {
  if (padding != RSA_PKCS1_PADDING) {
    fprintf(stderr, "Only PKCS1 padding is supported in %s\n", engine_id);
  }

  tob_key_t* key_info = (tob_key_t*)RSA_get_ex_data(rsa, rsa_ex_data_idx);
  return tob_key_encrypt(flen, from, to, key_info, RSA_WITH_PKCS1_PADDING);
}

// Decrypt with a private key
static int tob_engine_rsa_decrypt(int flen, const unsigned char* from, unsigned char* to, RSA* rsa, int padding) {
  if (padding != RSA_PKCS1_PADDING) {
    fprintf(stderr, "Only PKCS1 padding is supported in %s\n", engine_id);
  }

  tob_key_t* key_info = (tob_key_t*)RSA_get_ex_data(rsa, rsa_ex_data_idx);
  return tob_key_decrypt(flen, from, to, key_info, RSA_WITH_PKCS1_PADDING);
}

static RSA_METHOD* tob_engine_rsa(void) {
  static RSA_METHOD* tob_engine_meth = NULL;

  if (tob_engine_meth == NULL) {
    tob_engine_meth = RSA_meth_dup(RSA_get_default_method());
    RSA_meth_set1_name(tob_engine_meth, "Trust Onboard RSA method");
    RSA_meth_set_flags(tob_engine_meth, 0);
    // RSA_meth_set_priv_enc(tob_engine_meth, tob_engine_rsa_encrypt);
    RSA_meth_set_sign(tob_engine_meth, tob_engine_rsa_sign);
    RSA_meth_set_priv_dec(tob_engine_meth, tob_engine_rsa_decrypt);
    // RSA_meth_set_finish(tob_engine_meth, tob_engine_finish_method);
  }

  return tob_engine_meth;
}

static EVP_PKEY* tob_read_pubkey_ll(tob_ctx_t* ctx, uint8_t container_id) {
  uint16_t cert_len;
  EVP_PKEY* res = NULL;

  if (!ctx->mias->select(false)) {
    fprintf(stderr, "Couldn't select MIAS applet\n");
    return NULL;
  }

  if (!ctx->mias->verifyPin((uint8_t*)ctx->pin, strlen(ctx->pin))) {
    ctx->mias->deselect();
    return 0;
  }

  if (!ctx->mias->getCertificateByContainerId(container_id, NULL, &cert_len)) {
    fprintf(stderr, "Certificate with ID %d isn't found\n", (int)container_id);
    ctx->mias->deselect();
    return NULL;
  }

  uint8_t* cert = (uint8_t*)malloc(cert_len * sizeof(uint8_t));

  if (!cert) {
    fprintf(stderr, "Couldn't allocate memory for certificate\n");
    ctx->mias->deselect();
    return NULL;
  }

  if (!ctx->mias->getCertificateByContainerId(container_id, cert, &cert_len)) {
    fprintf(stderr, "Couldn't read certificate with ID %d\n", (int)container_id);
    ctx->mias->deselect();
    return NULL;
  }

  ctx->mias->deselect();

  BIO* cert_bio = BIO_new_mem_buf(cert, cert_len);
  if (!cert_bio) {
    fprintf(stderr, "Couldn't create BIO for the certificate\n");
    free(cert);
    return NULL;
  }

  X509* cert_x509 = d2i_X509_bio(cert_bio, NULL);
  BIO_free(cert_bio);
  free(cert);

  if (!cert_x509) {
    fprintf(stderr, "Couldn't parse DER certificate\n");
    return NULL;
  }

  res = X509_get_pubkey(cert_x509);

  X509_free(cert_x509);

  return res;
}

static EVP_PKEY* tob_engine_load_pubkey_function(ENGINE* engine, const char* name, UI_METHOD* ui_method,
                                                 void* callback_data) {
  (void)ui_method;
  (void)callback_data;

  tob_ctx_t* ctx = tob_get_ctx(engine);
  if (ctx == NULL) {
    return NULL;
  }
  unsigned long container_id = strtoul(name, NULL, 10);
  if (container_id > 255) {
    fprintf(stderr, "Invalid key name: %s\n", name);
    return NULL;
  }
  container_id &= 0xFF;

  return tob_read_pubkey_ll(ctx, container_id);
}

static EVP_PKEY* tob_engine_load_privkey_function(ENGINE* engine, const char* name, UI_METHOD* ui_method,
                                                  void* callback_data) {
  (void)ui_method;
  (void)callback_data;

  tob_ctx_t* ctx = tob_get_ctx(engine);
  if (ctx == NULL) {
    return NULL;
  }

  unsigned long container_id = strtoul(name, NULL, 10);
  if (container_id > 255) {
    fprintf(stderr, "Invalid key name: %s\n", name);
    return NULL;
  }
  container_id &= 0xFF;

  mias_key_pair_t* mias_key_pair = NULL;

  if (!ctx->mias->select(false)) {
    fprintf(stderr, "Couldn't select MIAS applet\n");
    return NULL;
  }

  if (!ctx->mias->verifyPin((uint8_t*)ctx->pin, strlen(ctx->pin))) {
    ctx->mias->deselect();
    return NULL;
  }

  if (!ctx->mias->getKeyPairByContainerId(container_id, &mias_key_pair) || mias_key_pair == NULL) {
    fprintf(stderr, "Key with ID %s isn't found\n", name);
    return NULL;
  }

  ctx->mias->deselect();

  EVP_PKEY* pubkey = tob_read_pubkey_ll(ctx, container_id);
  if (!pubkey) {
    return NULL;
  }

  RSA* pubkey_rsa = EVP_PKEY_get0_RSA(pubkey);
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
  rsa_key->e = BN_dup(pub_e);
  rsa_key->n = BN_dup(pub_n);
#endif

  EVP_PKEY_free(pubkey);

  if (rsa_ex_data_idx < 0) {
    rsa_ex_data_idx = RSA_get_ex_new_index(0, NULL, NULL, NULL, NULL);

    if (rsa_ex_data_idx < 0) {
      return NULL;
    }
  }

  tob_key_t* key_private = (tob_key_t*)malloc(sizeof(tob_key_t));
  key_private->kid       = mias_key_pair->kid;
  key_private->tob_ctx   = ctx;
  RSA_set_ex_data(rsa_key, rsa_ex_data_idx, key_private);

  RSA_set_method(rsa_key, tob_engine_rsa());

  EVP_PKEY* res = EVP_PKEY_new();

  if (!res) {
    RSA_free(rsa_key);
    return NULL;
  }


  EVP_PKEY_set1_RSA(res, rsa_key);
  RSA_free(rsa_key);
  return res;
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

  if (!ENGINE_set_destroy_function(e, tob_engine_destroy)) {
    fprintf(stderr, "ENGINE_set_destroy_function failed\n");
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

  if (!ENGINE_set_RSA(e, tob_engine_rsa())) {
    fprintf(stderr, "ENGINE_set_RSA failed\n");
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
