// Compiled into DOGEAR builds and into the host TLS tests (test/dogear_tls).
#if defined(DOGEAR_ENABLED) || defined(DOGEAR_TLS_HOST_TEST)

#include "DogearTls.h"

#include <wolfssl/wolfcrypt/settings.h>
// settings.h first: wolfSSL's headers depend on the user settings.
#include <wolfssl/ssl.h>

#include <cstring>

namespace dogear_tls {

bool configureContext(void* ctxPtr, const char* rootsPem, size_t rootsLen) {
  auto* ctx = static_cast<WOLFSSL_CTX*>(ctxPtr);
  if (!ctx || !rootsPem || rootsLen == 0) return false;
  // Verify the peer and refuse a server that sends no certificate. Set before
  // loading roots so that a load failure can never leave verification off.
  wolfSSL_CTX_set_verify(ctx, WOLFSSL_VERIFY_PEER | WOLFSSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
  const int loaded = wolfSSL_CTX_load_verify_buffer(ctx, reinterpret_cast<const unsigned char*>(rootsPem),
                                                    static_cast<long>(rootsLen), WOLFSSL_FILETYPE_PEM);
  return loaded == WOLFSSL_SUCCESS;
}

bool configureSession(void* sslPtr, const char* host) {
  auto* ssl = static_cast<WOLFSSL*>(sslPtr);
  if (!ssl || !host || host[0] == '\0') return false;
  // The certificate must name this host; without this a valid certificate
  // for any other site would be accepted.
  if (wolfSSL_check_domain_name(ssl, host) != WOLFSSL_SUCCESS) return false;
  return wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, host, static_cast<unsigned short>(strlen(host))) == WOLFSSL_SUCCESS;
}

}  // namespace dogear_tls

#endif
