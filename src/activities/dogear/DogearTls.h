#pragma once

#include <cstddef>

// DOGEAR's TLS trust policy, applied to a wolfSSL client. Plain wolfSSL calls,
// no Arduino, so the host tests run this exact code against real certificates.
//
// A verified session requires: the trusted roots load (fail closed if none
// do), the server presents a certificate chain to one of them, every
// certificate is in its validity period (the device clock must be set), and
// the certificate names the requested host (SAN or CN, wildcards per RFC 6125).
namespace dogear_tls {

// Opaque wolfSSL types (WOLFSSL_CTX*, WOLFSSL*) keep wolfSSL headers out of
// callers.
bool configureContext(void* ctx, const char* rootsPem, size_t rootsLen);
bool configureSession(void* ssl, const char* host);

}  // namespace dogear_tls
