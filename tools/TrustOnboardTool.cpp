/*
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

#include <stdio.h>

#include "BreakoutTrustOnboardSDK.h"

int main(int argc, char** argv) {
  int ret = 0;
  uint8_t cert[CERT_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t pk[PK_BUFFER_SIZE];
  int pk_size = 0;

  if (argc != 5) {
    printf("%s [device] [pin] [cert outfile] [key outfile]\n", argv[0]);
    printf("\nExample: %s /dev/ttyACM1 0000 certificate-chain.pem key.der\n", argv[0]);
    return -1;
  }

  char* device    = argv[1];
  char* pin       = argv[2];
  char* cert_path = argv[3];
  char* pk_path   = argv[4];

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

  return 0;
}
