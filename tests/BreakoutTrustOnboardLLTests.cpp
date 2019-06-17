/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <BreakoutTrustOnboardSDK.h>
#include <MIAS.h>
#include "GenericModem.h"
#ifdef PCSC_SUPPORT
#include "Pcsc.h"
#endif

#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#include <string>

static SEInterface* modem = nullptr;
static std::string pin    = "0000";

TEST_CASE("Select MIAS applet", "[mias][select]") {
  Applet::closeAllChannels(modem);

  auto mias = new MIAS();
  mias->init(modem);

  // TODO: select(true) is irreversible

  REQUIRE(mias->select(false));

  REQUIRE(mias->deselect());
}

TEST_CASE("Verify PIN on MIAS applet", "[mias][verifyPin]") {
  Applet::closeAllChannels(modem);

  auto mias = new MIAS();
  mias->init(modem);
  REQUIRE(mias->select(false));
  REQUIRE(mias->verifyPin((unsigned char*)pin.c_str(), pin.length()));
}

TEST_CASE("Container 0 has a valid key/certificate pair", "[mias][signingKeys]") {
  Applet::closeAllChannels(modem);

  auto mias = new MIAS();
  mias->init(modem);
  REQUIRE(mias->select(false));
  REQUIRE(mias->verifyPin((unsigned char*)pin.c_str(), pin.length()));

  mias_key_pair_t* keypair;
  // test that there is a key in container 0
  REQUIRE(mias->getKeyPairByContainerId(0x00, &keypair));
  REQUIRE(keypair != nullptr);

  uint8_t* cert;
  uint16_t cert_len;

  // test that there is a certificate in container 0
  uint16_t cert_len_probe;
  REQUIRE(mias->getCertificateByContainerId(0x00, nullptr, &cert_len_probe));
  REQUIRE(cert_len_probe != 0);

  cert = (uint8_t*)malloc(cert_len_probe * sizeof(uint8_t));
  REQUIRE(mias->getCertificateByContainerId(0x00, cert, &cert_len));
  REQUIRE(cert_len == cert_len_probe);

  EVP_PKEY* signing_pubkey = nullptr;
  // test that a public key can be extracted from the signing certificate
  BIO* cert_bio = BIO_new_mem_buf(cert, cert_len);
  REQUIRE(cert_bio != NULL);

  X509* cert_x509 = d2i_X509_bio(cert_bio, NULL);
  BIO_free(cert_bio);
  REQUIRE(cert_x509 != NULL);

  signing_pubkey = X509_get_pubkey(cert_x509);
  REQUIRE(signing_pubkey != nullptr);

  X509_free(cert_x509);

  // test that 2hat is signed with private key can be verified with public key
  int openssl_nids[]  = {NID_sha1, NID_sha224, NID_sha256, NID_sha384};
  uint8_t tob_algos[] = {ALGO_SHA1_WITH_RSA_PKCS1_PADDING, ALGO_SHA224_WITH_RSA_PKCS1_PADDING,
                         ALGO_SHA256_WITH_RSA_PKCS1_PADDING, ALGO_SHA384_WITH_RSA_PKCS1_PADDING};
  const char* message = "Humpty Dumpty sat on a wall";
  size_t message_len  = strlen(message);
  uint8_t signature[512];
  uint16_t signature_len;

  for (int i = 0; i < sizeof(openssl_nids) / sizeof(openssl_nids[0]); i++) {
    const EVP_MD* md      = EVP_get_digestbynid(openssl_nids[i]);
    int md_len            = EVP_MD_size(md);
    unsigned char* digest = (unsigned char*)malloc(md_len);

    REQUIRE(digest != NULL);

    EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
    REQUIRE(mdctx != nullptr);
    REQUIRE(EVP_DigestInit_ex(mdctx, md, NULL) == 1);
    REQUIRE(EVP_DigestUpdate(mdctx, message, message_len) == 1);
    REQUIRE(EVP_DigestFinal_ex(mdctx, digest, NULL) == 1);
    EVP_MD_CTX_destroy(mdctx);

    REQUIRE(mias->signInit(tob_algos[i], keypair->kid));
    REQUIRE(mias->signFinal(digest, md_len, signature, &signature_len));

    EVP_PKEY_CTX* evp_ctx = EVP_PKEY_CTX_new(signing_pubkey, ENGINE_by_id("openssl"));

    REQUIRE(evp_ctx != nullptr);
    REQUIRE(EVP_PKEY_verify_init(evp_ctx) > 0);
    REQUIRE(EVP_PKEY_CTX_set_rsa_padding(evp_ctx, RSA_PKCS1_PADDING) > 0);
    REQUIRE(EVP_PKEY_CTX_set_signature_md(evp_ctx, md) > 0);
    REQUIRE(EVP_PKEY_verify(evp_ctx, signature, signature_len, digest, md_len) == 1);
    EVP_PKEY_CTX_free(evp_ctx);
    free(digest);
  }

  // test that what is encrypted with public key can be decrypted with private key
  const char* plain = "Humpty Dumpty sat on a wall";
  size_t plain_len  = strlen(plain);
  unsigned char* cipher;
  size_t cipher_len;

  EVP_PKEY_CTX* pkctx = EVP_PKEY_CTX_new(signing_pubkey, nullptr);
  REQUIRE(pkctx != nullptr);
  REQUIRE(EVP_PKEY_encrypt_init(pkctx) > 0);
  REQUIRE(EVP_PKEY_CTX_set_rsa_padding(pkctx, RSA_PKCS1_PADDING) > 0);
  REQUIRE(EVP_PKEY_encrypt(pkctx, NULL, &cipher_len, (unsigned char*)plain, plain_len) > 0);
  cipher = (unsigned char*)malloc(cipher_len);
  REQUIRE(cipher != nullptr);
  REQUIRE(EVP_PKEY_encrypt(pkctx, cipher, &cipher_len, (unsigned char*)plain, plain_len) > 0);

  char* plain_tob = (char*)malloc(plain_len);
  uint16_t plain_len_tob;

  REQUIRE(mias->decryptInit(ALGO_RSA_PKCS1_PADDING, keypair->kid));
  REQUIRE(mias->decryptFinal(cipher, cipher_len, (unsigned char*)plain_tob, &plain_len_tob));
  REQUIRE(plain_len_tob == plain_len);
  REQUIRE(memcmp(plain, plain_tob, plain_len) == 0);
}

int main(int argc, char* argv[]) {
  Catch::Session session;

  std::string device;

  using namespace Catch::clara;
  auto cli = session.cli() |
             Opt(device, "device")["-m"]["--device"]("Path to the device or pcsc:N for a PC/SC interface") |
             Opt(pin, "pin")["-p"]["--pin"]("PIN code for the Trust Onboard SIM");

  // Now pass the new composite back to Catch so it uses that
  session.cli(cli);

  // Let Catch (using Clara) parse the command line
  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0)  // Indicates a command line error
    return returnCode;

  if (strncmp(device.c_str(), "pcsc:", 5) == 0) {
#ifdef PCSC_SUPPORT
    long idx = strtol(device.c_str() + 5, 0, 10);
    modem    = new PcscSEInterface((int)idx);
#else
    std::cerr << "No pcsc support, please rebuild with -DPCSC_SUPPORT=ON" << std::end;
    return 1;
#endif
  } else {
    modem = new GenericModem(device.c_str());
  }

  if (modem == nullptr || !modem->open()) {
    std::cerr << "Couldn't open modem at " << device << std::endl;
    return 1;
  }

  return session.run();
}
