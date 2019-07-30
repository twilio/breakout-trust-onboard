#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/x509_crt.h>
#include "BreakoutTrustOnboardSDK.h"
#include "TobMbedtls.h"

#include <string>
#include <iostream>

#define GET_REQUEST "GET %s HTTP/1.0\r\n\r\n"

static void print_usage() {
  std::cerr << "tob_client targetdomain.tld <port> /remote/resource/path [\"signing\"|\"available\"] "
               "/path/to/server/rootCA.pem <device_path> <device_baudrate> <mias_pin>"
            << std::endl;
}

static void debug_out(void* ctx, int level, const char* file, int line, const char* str) {
  ((void)level);

  fprintf((FILE*)ctx, "%s:%04d: %s", file, line, str);
  fflush((FILE*)ctx);
}

static bool map_file(const char* filename, unsigned char** buf, int* len) {
  struct stat sb;
  int fd = open(filename, O_RDONLY);

  if (fd < 0) {
    return false;
  }

  fstat(fd, &sb);
  *len = sb.st_size;

  *buf = (unsigned char*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);

  if (*buf == nullptr) {
    return false;
  }
  return true;
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

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_context ssl;
  mbedtls_ssl_config conf;
  mbedtls_x509_crt cacert;
  mbedtls_x509_crt device_cert;
  mbedtls_pk_context device_key;
  mbedtls_net_context server_fd;

  mbedtls_net_init(&server_fd);
  mbedtls_ssl_init(&ssl);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  mbedtls_x509_crt_init(&device_cert);
  mbedtls_pk_init(&device_key);

  unsigned char* cacert_buf;
  int cacert_buf_len;

  if (!map_file(root_ca, &cacert_buf, &cacert_buf_len)) {
    std::cerr << "Error opening server CA certificate" << std::endl;
    return 1;
  }

  if (mbedtls_x509_crt_parse(&cacert, cacert_buf, cacert_buf_len + 1) != 0) {
    std::cerr << "Error parsing server CA certificate" << std::endl;
    return 1;
  }

  mbedtls_entropy_init(&entropy);
  if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)"tob_client_sample",
                            strlen("tob_client_sample")) != 0) {
    return 1;
  }

  if (mbedtls_net_connect(&server_fd, url, port, MBEDTLS_NET_PROTO_TCP) != 0) {
    std::cerr << "mbedtls_net_connect failed" << std::endl;
    return 1;
  }

  if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                  MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
    std::cerr << "mbedtls_ssl_config_defaults failed" << std::endl;
    return 1;
  }

  mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
  mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
  mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
  mbedtls_ssl_conf_dbg(&conf, debug_out, stdout);

  tobInitialize(device_path, device_baudrate);

  uint8_t device_cert_buf[PEM_BUFFER_SIZE];
  int device_cert_buf_len;

  if (strcmp(client_key, "signing") == 0) {
    if (tobExtractSigningCertificate(nullptr, nullptr, device_cert_buf, &device_cert_buf_len, pin) != 0) {
      std::cerr << "Error extracting signing certificate" << std::endl;
      return 1;
    }

    if (!tob_mbedtls_setup_key(&device_key, pin, true)) {
      std::cerr << "Failed to initialize signing key" << std::endl;
      return 1;
    }
  } else if (strcmp(client_key, "available") == 0) {
    if (!tob_mbedtls_setup_key(&device_key, pin, false)) {
      std::cerr << "Failed to initialize available key" << std::endl;
      return 1;
    }

    if (tobExtractAvailableCertificate(device_cert_buf, &device_cert_buf_len, pin) != 0) {
      std::cerr << "Error extracting available certificate" << std::endl;
      return 1;
    }
  } else {
    std::cerr << "Invalid key name: " << client_key << std::endl;
    return 1;
  }

  if (mbedtls_x509_crt_parse(&device_cert, device_cert_buf, device_cert_buf_len + 1) != 0) {
    std::cerr << "Error parsing device certificate" << std::endl;
    return 1;
  }

  if (mbedtls_ssl_conf_own_cert(&conf, &device_cert, &device_key) != 0) {
    std::cerr << "mbedtls_ssl_conf_own_cert failed" << std::endl;
    return 1;
  }

  if (mbedtls_ssl_setup(&ssl, &conf) != 0) {
    std::cerr << "mbedtls_ssl_setup failed" << std::endl;
    return 1;
  }

  if (mbedtls_ssl_set_hostname(&ssl, url) != 0) {
    std::cerr << "mbedtls_ssl_set_hostname failed" << std::endl;
    return 1;
  }

  mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

  int ret;
  while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      std::cerr << "mbedtls_ssl_handshake failed" << std::endl;
      return 1;
    }
  }

  if (mbedtls_ssl_get_verify_result(&ssl) != 0) {
    std::cerr << "TLS verification failed" << std::endl;
    return 1;
  }

  char buf[1024];
  snprintf(buf, 1023, GET_REQUEST, resource);
  buf[1023] = 0;

  while (ret = mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf)) <= 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      std::cerr << "mbedtls_ssl_write failed" << std::endl;
      return 1;
    }
  }

  do {
    memset(buf, 0, sizeof(buf));
    int ret = mbedtls_ssl_read(&ssl, (unsigned char*)buf, sizeof(buf) - 1);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      continue;
    }

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
      break;
    }

    if (ret < 0) {
      break;
    }

    if (ret == 0) {
      break;
    }

    result += std::string(buf, ret);

  } while (1);

  mbedtls_ssl_close_notify(&ssl);


  std::cout << result;

  mbedtls_net_free(&server_fd);

  mbedtls_x509_crt_free(&cacert);
  mbedtls_x509_crt_free(&device_cert);
  mbedtls_pk_free(&device_key);
  mbedtls_ssl_free(&ssl);
  mbedtls_ssl_config_free(&conf);
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);

  return 0;
}
