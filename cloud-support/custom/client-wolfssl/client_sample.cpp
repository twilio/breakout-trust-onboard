#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/types.h>
#include "BreakoutTrustOnboardSDK.h"

#include <string>
#include <iostream>

#define GET_REQUEST "GET %s HTTP/1.0\r\n\r\n"

static void print_usage() {
  std::cerr << "tob_client targetdomain.tld <port> /remote/resource/path [\"available\"|\"signing\"] "
               "/path/to/server/rootCA.pem <device_path> <device_baudrate> <mias_pin>"
            << std::endl;
}

static char mias_pin[16];

static int tobRsaSign(WOLFSSL* ssl, const byte* in, word32 inSz, byte* out, word32* outSz, const byte* key,
                      word32 keySz, void* ctx) {
  //   The input is DigestInfo as per rfc3447:
  //
  //   DigestInfo ::= SEQUENCE {
  //     digestAlgorithm DigestAlgorithm,
  //     digest OCTET STRING
  //   }
  //
  //   mIAS takes only the digest as the input, so we need to extract it
  //
  if (inSz < 4) {
    return -1;
  }

  int da_len = in[3];  // skip wrapping sequence tag and length and DigestAlgoritm tag

  if (inSz < 4 + da_len + 2) {
    return -1;
  }

  int digest_len = in[4 + da_len + 1];
  if (inSz != 4 + da_len + 2 + digest_len) {
    return -1;
  }

  const unsigned char* digest = in + 4 + da_len + 2;

  tob_algorithm_t algo = (tob_algorithm_t)TOB_ALGO_RSA_PKCS1;  // WolfSSL doesn't support ISO9796 and RFC2409, mIAS
                                                               // doesn't support PSS, so only PKCS1 is in common

  switch (digest_len) {
    case 20:
      algo = (tob_algorithm_t)((int)algo | TOB_MD_SHA1);
      break;

    case 28:
      algo = (tob_algorithm_t)((int)algo | TOB_MD_SHA224);
      break;

    case 32:
      algo = (tob_algorithm_t)((int)algo | TOB_MD_SHA256);
      break;

    case 48:
      algo = (tob_algorithm_t)((int)algo | TOB_MD_SHA384);
      break;

    default:
      return -1;
  }

  int out_sz_int = 0;
  if (tobSigningSign(algo, digest, digest_len, out, &out_sz_int, mias_pin) != 0) {
    return -1;
  }

  *outSz = out_sz_int;
  return 0;
}

static int tobRsaSignCheck(WOLFSSL* ssl, byte* sig, word32 sigSz, byte** out, const byte* key, word32 keySz,
                           void* ctx) {
  return 0;
}

int main(int argc, const char** argv) {
  if (argc != 9) {
    print_usage();
    return 1;
  }

  const char* url         = argv[1];
  const char* port        = argv[2];
  const char* resource    = argv[3];
  const char* client_key  = argv[4];
  const char* root_ca     = argv[5];
  const char* device_path = argv[6];
  long device_baudrate    = strtol(argv[7], NULL, 10);
  const char* pin         = argv[8];

  std::string result;

  uint8_t cert[PEM_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t pkey[DER_BUFFER_SIZE];
  int pkey_size = 0;

  strncpy(mias_pin, pin, 15);
  mias_pin[15] = 0;


  if (tobInitialize(device_path, device_baudrate) != 0) {
    std::cerr << "Failed to initialized Trust Onboard" << std::endl;
    return 1;
  }

  wolfSSL_Init();

  WOLFSSL_CTX* ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());

  if (ctx == nullptr) {
    std::cerr << "Error creating WolfSSL context" << std::endl;
    return 1;
  }

  if (wolfSSL_CTX_load_verify_locations(ctx, root_ca, nullptr) != SSL_SUCCESS) {
    std::cerr << "Error opening server CA certificate" << std::endl;
    return 1;
  }

  if (strcmp(client_key, "signing") == 0) {
    if (tobExtractSigningCertificate(nullptr, nullptr, cert, &cert_size, pin) != 0) {
      std::cerr << "Error extracting signing certificate" << std::endl;
      return 1;
    }

    if (wolfSSL_CTX_use_certificate_buffer(ctx, cert, cert_size, SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
      std::cerr << "Error opening device certificate" << std::endl;
      return 1;
    }

    wolfSSL_CTX_SetRsaSignCb(ctx, tobRsaSign);
    wolfSSL_CTX_SetRsaSignCheckCb(ctx, tobRsaSignCheck);
  } else if (strcmp(client_key, "available") == 0) {
    if (tobExtractAvailableCertificate(cert, &cert_size, pin) != 0) {
      std::cerr << "Error extracting available certificate" << std::endl;
      return 1;
    }

    if (wolfSSL_CTX_use_certificate_buffer(ctx, cert, cert_size, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
      std::cerr << "Error opening device certificate" << std::endl;
      return 1;
    }

    if (tobExtractAvailablePrivateKey(nullptr, nullptr, pkey, &pkey_size, pin) != 0) {
      std::cerr << "Error extracting available private key" << std::endl;
      return 1;
    }

    if (wolfSSL_CTX_use_PrivateKey_buffer(ctx, pkey, pkey_size, SSL_FILETYPE_ASN1) != SSL_SUCCESS) {
      std::cerr << "Error opening device private key" << std::endl;
      return 1;
    }
  } else {
    std::cerr << "Unknown key name: " << client_key << std::endl;
    return 1;
  }

  WOLFSSL* ssl = wolfSSL_new(ctx);

  if (ssl == nullptr) {
    std::cerr << "Error creating WOLFSSL instance" << std::endl;
    return 1;
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    std::cerr << "Error opening socket" << std::endl;
    return 1;
  }

  struct hostent* he = gethostbyname(url);
  if (he == nullptr) {
    std::cerr << "Error resolving host name " << url << std::endl;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(sockaddr_in));
  long port_long         = strtol(port, nullptr, 10);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port   = htons((short)port_long);
  server_addr.sin_addr   = *((struct in_addr*)he->h_addr);

  if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "TCP connection failed" << std::endl;
    return 1;
  }

  wolfSSL_set_fd(ssl, socket_fd);


  char buf[1024];
  snprintf(buf, 1023, GET_REQUEST, resource);
  buf[1023] = 0;

  if (wolfSSL_write(ssl, (unsigned char*)buf, strlen(buf)) <= 0) {
    std::cerr << "wolfSSL_write failed" << std::endl;
    return 1;
  }

  do {
    memset(buf, 0, sizeof(buf));
    int ret = wolfSSL_read(ssl, (unsigned char*)buf, sizeof(buf) - 1);

    if (ret <= 0) {
      break;
    }

    result += std::string(buf, ret);

  } while (1);

  wolfSSL_free(ssl);
  wolfSSL_CTX_free(ctx);
  wolfSSL_Cleanup();

  std::cout << result;

  return 0;
}
