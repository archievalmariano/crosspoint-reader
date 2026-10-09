#pragma once

#include <cstddef>
#include <string>

#include "DogearNet.h"

// DOGEAR's own GET client: verified HTTPS (DogearTls + DogearTrustRoots),
// DOGEAR's redirect policy, and a byte cap. Used only by DOGEAR, so every other
// app keeps its existing transport.
namespace dogear_https {

enum class Result : uint8_t {
  Ok,
  Refused,        // the URL or a redirect breaks the policy (e.g. HTTPS -> HTTP)
  ClockUnset,     // no trustworthy time, so certificates cannot be checked
  ConnectFailed,  // TCP
  TlsFailed,      // handshake or certificate verification
  HttpError,      // status, framing, too many redirects, or over the cap
};

const char* describe(Result r);

// GET `url` into `out` (at most maxBytes of body). Only a complete 200 body
// counts as Ok.
Result get(const std::string& url, std::string& out, size_t maxBytes,
           dogear_net::Policy policy = dogear_net::kBuildPolicy);

}  // namespace dogear_https
