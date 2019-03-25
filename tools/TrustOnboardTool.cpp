/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include <stdio.h>

#include "BreakoutTrustOnboardSDK.h"

int main(int argc, char** argv) {
  int ret = 0;
  uint8_t cert[CERT_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t pk[PK_BUFFER_SIZE];
  int pk_size = 0;

  uint8_t signing_cert[CERT_BUFFER_SIZE];
  int signing_cert_size = 0;

  if (argc < 5) {
    printf("%s [device] [pin] [cert outfile] [key outfile] {[signing cert outfile]}\n", argv[0]);
    printf("\nExample: %s /dev/ttyACM1 0000 certificate-chain.pem key.der\n", argv[0]);
#ifdef PCSC_SUPPORT
    printf("\nExample (pcsclite): %s pcsc:0 0000 certificate-chain.pem key.der\n", argv[0]);
#endif
    return -1;
  }

  char* device            = argv[1];
  char* pin               = argv[2];
  char* cert_path         = argv[3];
  char* pk_path           = argv[4];
  char* signing_cert_path = (argc >= 6) ? argv[5] : nullptr;

  tobInitialize(device);

  ret = tobExtractAvailableCertificate(cert, &cert_size, pin);
  if (ret != 0) {
    printf("Error reading certificate chain: %d\n", ret);
    return -1;
  }

  ret = tobExtractAvailablePrivateKey(pk, &pk_size, pin);
  if (ret != 0) {
    printf("Error reading private key: %d\n", ret);
    return -1;
  }

  if (signing_cert_path != nullptr) {
    ret = tobExtractSigningCertificate(signing_cert, &signing_cert_size, pin);
    if (ret != 0) {
      printf("Error reading signing certificate chain: %d\n", ret);
      return -1;
    }
  }

  printf("Writing certificate chain with size: %d...\n", cert_size);
  FILE* cert_fp = fopen(cert_path, "w");
  if (!cert_fp) {
    printf("... error opening file for certificate output: %s\n", cert_path);
    return -1;
  }
  fwrite(cert, sizeof(uint8_t), cert_size, cert_fp);
  fclose(cert_fp);

  printf("Writing key with size: %d...\n", pk_size);
  FILE* pk_fp = fopen(pk_path, "w");
  if (!pk_fp) {
    printf("... error opening file for pk output: %s\n", pk_path);
    return -1;
  }
  fwrite(pk, sizeof(uint8_t), pk_size, pk_fp);
  fclose(pk_fp);

  if (signing_cert_path != nullptr) {
    printf("Writing signing certificate chain with size: %d...\n", signing_cert_size);
    FILE* signing_cert_fp = fopen(signing_cert_path, "w");
    if (!signing_cert_fp) {
      printf("... error opening file for signing certificate output: %s\n", signing_cert_path);
      return -1;
    }
    fwrite(signing_cert, sizeof(uint8_t), signing_cert_size, signing_cert_fp);
    fclose(signing_cert_fp);
  }

  return 0;
}
