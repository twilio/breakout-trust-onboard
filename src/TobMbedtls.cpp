#include "TobMbedtls.h"
#include "TobMbedtlsLL.h"
#include <stdlib.h>

#include "MIAS.h"
#include "GenericModem.h"

#ifdef PCSC_SUPPORT
#include "Pcsc.h"
#endif

bool tob_mbedtls_init(const char* iface_path, const char* pin) {
  if (iface_path == nullptr) {
    return false;
  }

  SEInterface* iface;
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

  return tob_mbedtls_init_ll(iface, pin);
}

bool tob_mbedtls_signing_key(mbedtls_pk_context* pk, uint8_t key_id) {
  return tob_mbedtls_signing_key_ll(pk, key_id);
}
