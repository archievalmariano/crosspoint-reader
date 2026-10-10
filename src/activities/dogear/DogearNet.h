#pragma once

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <string>

// DOGEAR's transport policy (pure, host-testable).
//
// Production fetches are HTTPS only, verified against DogearTrustRoots.h with
// the hostname checked (DogearTls). Plain HTTP is a development exception: a
// build made with -DDOGEAR_ALLOW_LAN_HTTP may fetch from a private LAN address
// (RFC 1918 IPv4 or *.local), never from a public host. Redirects never go
// from HTTPS to HTTP, whatever the build.
namespace dogear_net {

enum class Policy : uint8_t { HttpsOnly, AllowLanHttp };

#if defined(DOGEAR_ALLOW_LAN_HTTP)
inline constexpr Policy kBuildPolicy = Policy::AllowLanHttp;
#else
inline constexpr Policy kBuildPolicy = Policy::HttpsOnly;
#endif

inline constexpr int kMaxRedirects = 3;

// Certificate validity needs a real clock. Anything before this (2026-01-01
// UTC) means the clock was never set, and verification must not run on it.
inline constexpr time_t kMinPlausibleUnixTime = 1767225600;
inline bool clockPlausible(time_t now) { return now >= kMinPlausibleUnixTime; }

struct Url {
  bool https = false;
  std::string host;  // lowercase
  uint16_t port = 0;
  std::string path;  // starts with '/'
};

inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

// A DNS name (letters, digits, '-', '.'; labels 1-63; no leading/trailing
// '-' or '.') or a dotted IPv4 address. Lowercases `host` in place.
inline bool validHost(std::string& host) {
  if (host.empty() || host.size() > 253) return false;
  size_t label = 0;
  char prev = '.';
  for (char& c : host) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    const bool alnum = (c >= 'a' && c <= 'z') || isDigit(c);
    if (c == '.') {
      if (label == 0 || prev == '-') return false;
      label = 0;
    } else if (alnum || c == '-') {
      if (c == '-' && label == 0) return false;
      if (++label > 63) return false;
    } else {
      return false;
    }
    prev = c;
  }
  return label > 0 && prev != '-';
}

// Four dotted decimal octets, each 0-255.
inline bool parseIpv4(const std::string& host, int octets[4]) {
  int n = 0;
  size_t i = 0;
  while (n < 4) {
    if (i >= host.size() || !isDigit(host[i])) return false;
    int v = 0;
    size_t digits = 0;
    while (i < host.size() && isDigit(host[i])) {
      v = v * 10 + (host[i++] - '0');
      if (++digits > 3 || v > 255) return false;
    }
    octets[n++] = v;
    if (n < 4) {
      if (i >= host.size() || host[i] != '.') return false;
      ++i;
    }
  }
  return i == host.size();
}

inline bool isPrivateLanHost(const std::string& host) {
  int o[4];
  if (parseIpv4(host, o)) {
    return o[0] == 10 || (o[0] == 172 && o[1] >= 16 && o[1] <= 31) || (o[0] == 192 && o[1] == 168);
  }
  constexpr char kLocal[] = ".local";
  return host.size() > sizeof(kLocal) - 1 &&
         host.compare(host.size() - (sizeof(kLocal) - 1), sizeof(kLocal) - 1, kLocal) == 0;
}

// Strict absolute http(s) URL: no userinfo, no spaces or control characters,
// an explicit port in 1-65535 if given.
inline bool parseUrl(const std::string& s, Url& out) {
  if (std::any_of(s.begin(), s.end(), [](char c) { return static_cast<unsigned char>(c) <= 0x20 || c == 0x7f; })) {
    return false;
  }
  size_t rest;
  if (s.rfind("https://", 0) == 0) {
    out.https = true;
    rest = 8;
  } else if (s.rfind("http://", 0) == 0) {
    out.https = false;
    rest = 7;
  } else {
    return false;
  }
  const size_t slash = s.find_first_of("/?#", rest);
  std::string authority = s.substr(rest, slash == std::string::npos ? std::string::npos : slash - rest);
  if (authority.find('@') != std::string::npos) return false;
  out.port = out.https ? 443 : 80;
  const size_t colon = authority.find(':');
  if (colon != std::string::npos) {
    const std::string port = authority.substr(colon + 1);
    if (port.empty() || port.size() > 5) return false;
    uint32_t p = 0;
    for (const char c : port) {
      if (!isDigit(c)) return false;
      p = p * 10 + static_cast<uint32_t>(c - '0');
    }
    if (p == 0 || p > 65535) return false;
    out.port = static_cast<uint16_t>(p);
    authority.resize(colon);
  }
  if (!validHost(authority)) return false;
  out.host = authority;
  out.path = slash == std::string::npos ? "/" : s.substr(slash);
  if (out.path[0] != '/') out.path.insert(0, "/");  // "?q" -> "/?q"
  const size_t hash = out.path.find('#');
  if (hash != std::string::npos) out.path.resize(hash);
  return true;
}

inline bool urlAllowed(const Url& url, Policy policy) {
  if (url.https) return true;
  return policy == Policy::AllowLanHttp && isPrivateLanHost(url.host);
}

// The target of a redirect from `base`: an absolute http(s) URL or an
// absolute path on the same origin. Anything else is refused.
inline bool resolveRedirect(const Url& base, const std::string& location, Url& out) {
  if (location.rfind("https://", 0) == 0 || location.rfind("http://", 0) == 0) return parseUrl(location, out);
  if (location.empty() || location[0] != '/' || location.rfind("//", 0) == 0) return false;
  std::string url = std::string(base.https ? "https://" : "http://") + base.host;
  if (base.port != (base.https ? 443 : 80)) url += ":" + std::to_string(base.port);
  return parseUrl(url + location, out);
}

// HTTPS never redirects to HTTP; the target must itself be allowed.
inline bool redirectAllowed(const Url& from, const Url& to, Policy policy) {
  if (from.https && !to.https) return false;
  return urlAllowed(to, policy);
}

}  // namespace dogear_net
