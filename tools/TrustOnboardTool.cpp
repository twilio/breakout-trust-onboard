/*
 *
 * Twilio Breakout Trust Onboard SDK
 *
 * Copyright (c) 2019 Twilio, Inc.
 *
 * SPDX-License-Identifier:  Apache-2.0
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

// nlohmann's single-header implementation
#include <json.hpp>

#include "BreakoutTrustOnboardSDK.h"

#define MAX_BAUDRATE 4000000
void print_usage(const char* program_name) {
  fprintf(stderr, "%s <arguments>\n", program_name);
  fprintf(stderr,
          "\nRequired arguments:\n\
  -d,--device=<device>          - device to connect to a SIM, can be either a\n\
                                  char device under /dev or of form pcsc:N if\n\
				  built with PC/SC interface support\n\
  -p,--pin=<pin>                - PIN code for the mIAS applet on your SIM card\n\
\n\
Optional arguments:\n\
  -b,--baudrate=<baudrate>      - baud rate for a serial device. Defaults to\n\
                                  115200\n\
  -a,--available-cert=<cert>    - path to store the available certificate\n\
  -k,--available-key=<key>      - path to store the available private key\n\
  -s,--signing-cert=<cert>      - path to store the signing certifiate\n\
  -j,--json                     - print everything as a json object\n\
\n\
Examples:\n");
  fprintf(stderr, "\n%s -d /dev/ttyACM1 -p 0000 -a certificate.pem -k key.pem\n", program_name);
  fprintf(stderr, "\n%s -d /dev/ttyUSB0 -b 9600 -p 0000 -s signing-certificate.pem\n", program_name);
#ifdef PCSC_SUPPORT
  fprintf(stderr, "\n%s --device=pcsc:0 --pin=0000 --available-cert=certificate-chain.pem --available-key=key.pem\n",
          program_name);
#endif
}

int main(int argc, char** argv) {
  int ret         = 0;
  int cert_size   = 0;
  int pk_size     = 0;
  int pk_size_der = 0;

  int signing_cert_size     = 0;
  int signing_cert_size_der = 0;

  uint8_t* cert;
  uint8_t* pk;
  uint8_t* pk_der;
  uint8_t* signing_cert;
  uint8_t* signing_cert_der;

  char* device            = nullptr;
  int baudrate            = 115200;
  char* pin               = nullptr;
  char* cert_path         = nullptr;
  char* pk_path           = nullptr;
  char* signing_cert_path = nullptr;
  bool print_json         = false;

  static struct option options[] = {{"device", required_argument, NULL, 'd'},
                                    {"baudrate", required_argument, NULL, 'b'},
                                    {"pin", required_argument, NULL, 'p'},
                                    {"available-cert", required_argument, NULL, 'a'},
                                    {"available-key", required_argument, NULL, 'k'},
                                    {"signing-cert", required_argument, NULL, 's'},
                                    {"json", no_argument, NULL, 'j'},
                                    {NULL, 0, NULL, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "d:b:p:a:k:s:j", options, NULL)) != -1) {
    switch (opt) {
      case 'd':
        device = optarg;
        break;
      case 'b': {
        long long_baudrate = strtol(optarg, nullptr, 10);
        if (long_baudrate < 0 || long_baudrate >= MAX_BAUDRATE) {
          fprintf(stderr, "Invalid baudrate: %s\n", optarg);
          print_usage(argv[0]);
          return 1;
        }
        baudrate = (int)long_baudrate;
        break;
      }
      case 'p':
        pin = optarg;
        break;
      case 'a':
        cert_path = optarg;
        break;
      case 'k':
        pk_path = optarg;
        break;
      case 's':
        signing_cert_path = optarg;
        break;
      case 'j':
        print_json = true;
        break;
      default:
        fprintf(stderr, "Invalid option: %c\n", opt);
        print_usage(argv[0]);
        return 1;
    }
  }

  if (device == nullptr) {
    fprintf(stderr, "Device is not set\n");
    print_usage(argv[0]);
    return 1;
  }


  if (pin == nullptr) {
    fprintf(stderr, "PIN is not set\n");
    print_usage(argv[0]);
    return 1;
  }

  tobInitialize(device, baudrate);

  ret = tobExtractAvailableCertificate(NULL, &cert_size, pin);
  if (ret != 0) {
    fprintf(stderr, "Error reading available certificate length: %d\n", ret);
    return 1;
  }

  cert = (uint8_t*)malloc(cert_size * sizeof(uint8_t));

  if (cert == NULL) {
    fprintf(stderr, "Failed to allocate memory for the available certificate\n");
    return 1;
  }

  ret = tobExtractAvailableCertificate(cert, &cert_size, pin);
  if (ret != 0) {
    fprintf(stderr, "Error reading available certificate: %d\n", ret);
    return 1;
  }

  if (cert_path != nullptr) {
    fprintf(stderr, "Writing certificate chain with size: %d...\n", cert_size);
    FILE* cert_fp = fopen(cert_path, "w");
    if (!cert_fp) {
      fprintf(stderr, "... error opening file for certificate output: %s\n", cert_path);
      return -1;
    }
    fwrite(cert, sizeof(uint8_t), cert_size, cert_fp);
    fclose(cert_fp);
  }

  ret = tobExtractAvailablePrivateKey(NULL, &pk_size, NULL, &pk_size_der, pin);
  if (ret != 0) {
    fprintf(stderr, "Error reading private key size: %d\n", ret);
    return -1;
  }

  pk     = (uint8_t*)malloc(pk_size * sizeof(uint8_t));
  pk_der = (uint8_t*)malloc(pk_size_der * sizeof(uint8_t));

  if (pk == NULL || pk_der == NULL) {
    fprintf(stderr, "Failed to allocate memory for the available private key\n");
    return 1;
  }

  ret = tobExtractAvailablePrivateKey(pk, &pk_size, pk_der, &pk_size_der, pin);
  if (ret != 0) {
    fprintf(stderr, "Error reading private key: %d\n", ret);
    return 1;
  }

  if (pk_path != nullptr) {
    fprintf(stderr, "Writing key with size: %d...\n", pk_size);
    FILE* pk_fp = fopen(pk_path, "w");
    if (!pk_fp) {
      fprintf(stderr, "... error opening file for pk output: %s\n", pk_path);
      return -1;
    }
    fwrite(pk, sizeof(uint8_t), pk_size, pk_fp);
    fclose(pk_fp);
  }

  ret = tobExtractSigningCertificate(NULL, &signing_cert_size, NULL, &signing_cert_size_der, pin);
  if (ret != 0) {
    fprintf(stderr, "Error reading signing certificate length: %d\n", ret);
    return -1;
  }

  signing_cert     = (uint8_t*)malloc(signing_cert_size * sizeof(uint8_t));
  signing_cert_der = (uint8_t*)malloc(signing_cert_size_der * sizeof(uint8_t));

  if (signing_cert == NULL || signing_cert_der == NULL) {
    fprintf(stderr, "Failed to allocate memory for signing certificate\n");
    return 1;
  }

  ret = tobExtractSigningCertificate(signing_cert, &signing_cert_size, signing_cert_der, &signing_cert_size_der, pin);
  if (ret != 0) {
    fprintf(stderr, "Error reading signing certificate: %d\n", ret);
    return -1;
  }

  if (signing_cert_path != nullptr) {
    fprintf(stderr, "Writing signing certificate chain with size: %d...\n", signing_cert_size);
    FILE* signing_cert_fp = fopen(signing_cert_path, "w");
    if (!signing_cert_fp) {
      fprintf(stderr, "... error opening file for signing certificate output: %s\n", signing_cert_path);
      return -1;
    }
    fwrite(signing_cert, sizeof(uint8_t), signing_cert_size, signing_cert_fp);
    fclose(signing_cert_fp);
  }

  if (print_json) {
    nlohmann::json j;
    j["available_certificate"] = std::string((const char*)cert);
    j["available_pkey"]        = std::string((const char*)pk);
    j["signing_certificate"]   = std::string((const char*)signing_cert);

    std::cout << j.dump() << std::endl;
  }

  fprintf(stderr, "All done.\n");
  return 0;
}
