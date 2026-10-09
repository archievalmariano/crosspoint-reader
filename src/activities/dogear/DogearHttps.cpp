// Compiled only into builds that enable DOGEAR (see [dogear] in platformio.ini).
#ifdef DOGEAR_ENABLED

#include "DogearHttps.h"

#include <Arduino.h>
#include <Logging.h>
#include <WiFiClient.h>
#include <wolfssl/wolfcrypt/settings.h>
// settings.h first: wolfSSL's headers depend on the user settings.
#include <wolfssl/ssl.h>

#include <cstdlib>
#include <ctime>

#include "DogearHttpParser.h"
#include "DogearTls.h"
#include "DogearTrustRoots.h"

namespace dogear_https {
namespace {

constexpr uint32_t kTimeoutMs = 15000;
constexpr uint32_t kClockSyncWaitMs = 6000;
constexpr size_t kReadChunk = 256;  // the stack-buffer guideline

// wolfSSL I/O over a WiFiClient, as in SecureNet's SecureClient.
int ioSend(WOLFSSL*, char* buf, int sz, void* ctx) {
  auto* t = static_cast<WiFiClient*>(ctx);
  const int n = t->write(reinterpret_cast<const uint8_t*>(buf), sz);
  if (n <= 0) return t->connected() ? WOLFSSL_CBIO_ERR_WANT_WRITE : WOLFSSL_CBIO_ERR_CONN_CLOSE;
  return n;
}
int ioRecv(WOLFSSL*, char* buf, int sz, void* ctx) {
  auto* t = static_cast<WiFiClient*>(ctx);
  if (t->available() == 0) return t->connected() ? WOLFSSL_CBIO_ERR_WANT_READ : WOLFSSL_CBIO_ERR_CONN_CLOSE;
  const int n = t->read(reinterpret_cast<uint8_t*>(buf), sz);
  return n > 0 ? n : WOLFSSL_CBIO_ERR_WANT_READ;
}
bool wantIo(int err) { return err == WOLFSSL_ERROR_WANT_READ || err == WOLFSSL_ERROR_WANT_WRITE; }

// Certificate dates need real time. The X4 has no RTC, so its clock may still
// read 1970: set it from NTP first, and refuse TLS if that fails.
bool ensureClock() {
  if (dogear_net::clockPlausible(time(nullptr))) return true;
  const char* tz = getenv("TZ");
  char savedTz[64] = {0};
  if (tz) snprintf(savedTz, sizeof(savedTz), "%s", tz);
  configTzTime("UTC0", "pool.ntp.org", "time.google.com");
  const uint32_t start = millis();
  while (!dogear_net::clockPlausible(time(nullptr)) && millis() - start < kClockSyncWaitMs) delay(100);
  if (savedTz[0]) {
    setenv("TZ", savedTz, 1);
  } else {
    unsetenv("TZ");
  }
  tzset();
  const bool ok = dogear_net::clockPlausible(time(nullptr));
  if (!ok) LOG_ERR("DOGEAR", "clock not set (no NTP); refusing TLS");
  return ok;
}

// One connection: plain (LAN dev only) or verified TLS. Owns its wolfSSL state.
class Connection {
 public:
  ~Connection() { close(); }

  Result open(const dogear_net::Url& url) {
    tcp.setConnectionTimeout(kTimeoutMs);
    if (!tcp.connect(url.host.c_str(), url.port)) return Result::ConnectFailed;
    if (!url.https) return Result::Ok;

    ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
    if (!ctx || !dogear_tls::configureContext(ctx, kDogearTrustRoots, sizeof(kDogearTrustRoots) - 1)) {
      LOG_ERR("DOGEAR", "TLS trust setup failed; refusing");
      return Result::TlsFailed;
    }
    wolfSSL_SetIORecv(ctx, ioRecv);
    wolfSSL_SetIOSend(ctx, ioSend);
    ssl = wolfSSL_new(ctx);
    if (!ssl || !dogear_tls::configureSession(ssl, url.host.c_str())) {
      LOG_ERR("DOGEAR", "TLS session setup failed; refusing");
      return Result::TlsFailed;
    }
    wolfSSL_SetIOReadCtx(ssl, &tcp);
    wolfSSL_SetIOWriteCtx(ssl, &tcp);
#if defined(WOLFSSL_TLS13) && defined(HAVE_CURVE25519)
    wolfSSL_UseKeyShare(ssl, WOLFSSL_ECC_X25519);  // small key share; see SecureClient
#endif
#ifdef HAVE_MAX_FRAGMENT
    wolfSSL_UseMaxFragment(ssl, WOLFSSL_MFL_2_11);  // 2 KB records keep the receive buffer small
#endif
    const uint32_t deadline = millis() + kTimeoutMs;
    int ret;
    while ((ret = wolfSSL_connect(ssl)) != WOLFSSL_SUCCESS) {
      const int err = wolfSSL_get_error(ssl, ret);
      if (!wantIo(err) || static_cast<int32_t>(millis() - deadline) >= 0) {
        LOG_ERR("DOGEAR", "TLS handshake with %s failed: %d", url.host.c_str(), err);
        return Result::TlsFailed;
      }
      delay(2);
    }
    LOG_INF("DOGEAR", "verified TLS to %s (%s)", url.host.c_str(), wolfSSL_get_version(ssl));
    return Result::Ok;
  }

  bool send(const std::string& data) {
    if (!ssl) return tcp.write(reinterpret_cast<const uint8_t*>(data.data()), data.size()) == data.size();
    size_t sent = 0;
    const uint32_t deadline = millis() + kTimeoutMs;
    while (sent < data.size()) {
      const int n = wolfSSL_write(ssl, data.data() + sent, static_cast<int>(data.size() - sent));
      if (n > 0) {
        sent += static_cast<size_t>(n);
      } else if (!wantIo(wolfSSL_get_error(ssl, n)) || static_cast<int32_t>(millis() - deadline) >= 0) {
        return false;
      }
    }
    return true;
  }

  // >0 bytes read, 0 nothing yet, <0 closed or failed.
  int receive(char* buf, size_t len) {
    if (!ssl) {
      if (tcp.available() > 0) return tcp.read(reinterpret_cast<uint8_t*>(buf), len);
      return tcp.connected() ? 0 : -1;
    }
    const int n = wolfSSL_read(ssl, buf, static_cast<int>(len));
    if (n > 0) return n;
    const int err = wolfSSL_get_error(ssl, n);
    if (wantIo(err)) return tcp.connected() || tcp.available() > 0 ? 0 : -1;
    return -1;  // close_notify or failure
  }

  void close() {
    if (ssl) wolfSSL_free(ssl);
    if (ctx) wolfSSL_CTX_free(ctx);
    ssl = nullptr;
    ctx = nullptr;
    tcp.stop();
  }

 private:
  WiFiClient tcp;
  WOLFSSL_CTX* ctx = nullptr;
  WOLFSSL* ssl = nullptr;
};

}  // namespace

const char* describe(Result r) {
  switch (r) {
    case Result::Ok:
      return "ok";
    case Result::Refused:
      return "refused by policy";
    case Result::ClockUnset:
      return "clock not set";
    case Result::ConnectFailed:
      return "connect failed";
    case Result::TlsFailed:
      return "TLS verification failed";
    case Result::HttpError:
      return "HTTP error";
  }
  return "?";
}

Result get(const std::string& startUrl, std::string& out, size_t maxBytes, dogear_net::Policy policy) {
  out.clear();
  dogear_net::Url url;
  if (!dogear_net::parseUrl(startUrl, url) || !dogear_net::urlAllowed(url, policy)) {
    LOG_ERR("DOGEAR", "refusing %s", startUrl.c_str());
    return Result::Refused;
  }

  for (int hop = 0; hop <= dogear_net::kMaxRedirects; ++hop) {
    if (url.https && !ensureClock()) return Result::ClockUnset;
    Connection conn;
    const Result opened = conn.open(url);
    if (opened != Result::Ok) return opened;

    std::string request = "GET " + url.path + " HTTP/1.1\r\nHost: " + url.host;
    if (url.port != (url.https ? 443 : 80)) request += ":" + std::to_string(url.port);
    request += "\r\nUser-Agent: CrossPoint-ESP32-" CROSSPOINT_VERSION
               "\r\nAccept: application/json\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
    if (!conn.send(request)) return Result::HttpError;

    dogear_http::ResponseParser parser(maxBytes);
    char buf[kReadChunk];
    uint32_t lastData = millis();
    bool eof = false;
    while (!parser.done() && !parser.failed()) {
      // A redirect needs only its headers.
      if (parser.headersDone() && parser.isRedirect()) break;
      const int n = conn.receive(buf, sizeof(buf));
      if (n > 0) {
        lastData = millis();
        if (!parser.feed(buf, static_cast<size_t>(n))) break;
      } else if (n < 0) {
        eof = true;
        break;
      } else if (millis() - lastData > kTimeoutMs) {
        break;
      } else {
        delay(2);
      }
    }
    if (eof) parser.finishAtEof();
    if (parser.failed() || !parser.headersDone()) {
      LOG_ERR("DOGEAR", "bad or oversized response from %s", url.host.c_str());
      return Result::HttpError;
    }

    if (parser.isRedirect()) {
      dogear_net::Url next;
      if (!dogear_net::resolveRedirect(url, parser.location(), next) ||
          !dogear_net::redirectAllowed(url, next, policy)) {
        LOG_ERR("DOGEAR", "refusing redirect from %s to %s", url.host.c_str(), parser.location().c_str());
        return Result::Refused;
      }
      url = next;
      continue;
    }
    if (parser.status() != 200 || !parser.done()) {
      LOG_ERR("DOGEAR", "GET %s: status %d%s", url.path.c_str(), parser.status(), parser.done() ? "" : " (incomplete)");
      return Result::HttpError;
    }
    out = parser.body();
    return Result::Ok;
  }
  LOG_ERR("DOGEAR", "too many redirects");
  return Result::HttpError;
}

}  // namespace dogear_https

#endif  // DOGEAR_ENABLED
