#include <stdio.h>

#include "CinterionModem.h"
#include "breakout_tob.h"

#include "config.h"

int main(int argc, char** argv) {
  int ret = 0;
  uint8_t cert[CERT_BUFFER_SIZE];
  int cert_size = 0;
  uint8_t pk[PK_BUFFER_SIZE];
  int pk_size = 0;

  if (argc != 5) {
    printf("%s [device] [pin] [cert outfile] [key outfile]\n", argv[0]);
    printf("\nExample: %s /dev/ttyACM1 0000 certificate.p11 key.der\n", argv[0]);
    return -1;
  }

  char* device = argv[1];
  char* pin = argv[2];
  char* cert_path = argv[3];
  char* pk_path = argv[4];

  CinterionModem modem(device);

	if(!modem.open()) {
		printf("Error modem not found!\n");
		return -1;
	}
	mbedtls_se_init(&modem);  

  ret = mbedtls_x509_crt_parse_se(cert, &cert_size, CERT_MIAS_PATH, pin);
  if (ret != 0) {
    printf("Error reading certificate: %d\n", ret);
    return -1;
  }

  ret = mbedtls_pk_parse_se(pk, &pk_size, PK_MIAS_PATH, pin);
  if (ret != 0) {
    printf("Error reading private key: %d\n", ret);
    return -1;
  }

  // # display certificate information
  // cat certificate.p11 | openssl x509 -text -noout -inform p11
  //
  // # convert p11 cert to pem
  // openssl x509 -inform p11 -in certificate.p11 -out credential.pem
  // # convert der key to pem
  // openssl rsa -inform der -in key.der -outform pem -out key.pem
  // # build p12 from cert and key
  // openssl pkcs12 -export -out credential.pfx -inkey key.pem -in credential.pem -certfile programmable-wireless.available.bundle -password pass:changeit
  //
  // # verify hashes match:
  // openssl x509 -noout -modulus -in credential.pem | openssl md5
  // openssl rsa -noout -modulus -in key.pem | openssl md5

  printf("Writing p11 certificate with size: %d...\n", cert_size);
  FILE *cert_fp = fopen(cert_path, "w");
  if (!cert_fp) {
    printf("... error opening file for certificate output: %s\n", cert_path);
    return -1;
  }
  fwrite(cert, sizeof(uint8_t), cert_size, cert_fp);
  fclose(cert_fp);

  printf("Writing key with size: %d...\n", pk_size);
  FILE *pk_fp = fopen(pk_path, "w");
  if (!pk_fp) {
    printf("... error opening file for pk output: %s\n", pk_path);
    return -1;
  }
  fwrite(pk, sizeof(uint8_t), pk_size, pk_fp);
  fclose(pk_fp);

  return 0;
}
