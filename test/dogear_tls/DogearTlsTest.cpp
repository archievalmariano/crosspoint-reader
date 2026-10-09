// DOGEAR's TLS policy against real certificates, with the firmware's wolfSSL
// build (see CMakeLists.txt). Client and server run in one thread over a
// non-blocking socket pair, so no network is needed. The opt-in live test
// (DOGEAR_TLS_LIVE=1) also checks the production roots against real hosts.
#include <arpa/inet.h>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wolfssl/wolfcrypt/settings.h>
// settings.h first: wolfSSL's headers depend on the user settings.
#include <ArduinoJson.h>
#include <wolfssl/error-ssl.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "DogearCache.h"
#include "DogearIssue.h"
#include "DogearLimits.h"
#include "DogearTls.h"
#include "DogearTrustRoots.h"
#include "TestSha256.h"

namespace {

std::string cert(const std::string& name) {
  std::ifstream f(std::string(DOGEAR_TEST_CERTS) + "/" + name);
  std::stringstream ss;
  ss << f.rdbuf();
  EXPECT_FALSE(ss.str().empty()) << name;
  return ss.str();
}

class WolfEnv : public ::testing::Environment {
 public:
  void SetUp() override { ASSERT_EQ(wolfSSL_Init(), WOLFSSL_SUCCESS); }
  void TearDown() override { wolfSSL_Cleanup(); }
};
const auto* const kEnv = ::testing::AddGlobalTestEnvironment(new WolfEnv);

struct Outcome {
  bool configured = false;  // the trust policy applied
  bool connected = false;   // handshake completed and verified
  int error = 0;            // the client's wolfSSL error on failure
};

// A server presenting `leaf`, and a DOGEAR-configured client trusting `roots`
// and asking for `host`.
Outcome handshake(const std::string& leaf, const std::string& key, const std::string& roots, const char* host) {
  Outcome out;
  int sv[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) return out;
  fcntl(sv[0], F_SETFL, O_NONBLOCK);
  fcntl(sv[1], F_SETFL, O_NONBLOCK);

  WOLFSSL_CTX* sctx = wolfSSL_CTX_new(wolfSSLv23_server_method());
  WOLFSSL_CTX* cctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
  EXPECT_EQ(wolfSSL_CTX_use_certificate_buffer(sctx, reinterpret_cast<const unsigned char*>(leaf.data()),
                                               static_cast<long>(leaf.size()), WOLFSSL_FILETYPE_PEM),
            WOLFSSL_SUCCESS);
  EXPECT_EQ(wolfSSL_CTX_use_PrivateKey_buffer(sctx, reinterpret_cast<const unsigned char*>(key.data()),
                                              static_cast<long>(key.size()), WOLFSSL_FILETYPE_PEM),
            WOLFSSL_SUCCESS);
  WOLFSSL* server = wolfSSL_new(sctx);
  WOLFSSL* client = nullptr;
  out.configured = dogear_tls::configureContext(cctx, roots.data(), roots.size());
  if (out.configured) {
    client = wolfSSL_new(cctx);
    out.configured = dogear_tls::configureSession(client, host);
  }
  if (out.configured) {
    wolfSSL_set_fd(client, sv[0]);
    wolfSSL_set_fd(server, sv[1]);
    bool serverDone = false;
    for (int i = 0; i < 1000 && !out.connected; ++i) {
      const int rc = wolfSSL_connect(client);
      if (rc == WOLFSSL_SUCCESS) {
        out.connected = true;
        break;
      }
      const int err = wolfSSL_get_error(client, rc);
      if (err != WOLFSSL_ERROR_WANT_READ && err != WOLFSSL_ERROR_WANT_WRITE) {
        out.error = err;
        break;
      }
      if (!serverDone) {
        const int rs = wolfSSL_accept(server);
        serverDone = rs == WOLFSSL_SUCCESS;
        const int serr = wolfSSL_get_error(server, rs);
        if (!serverDone && serr != WOLFSSL_ERROR_WANT_READ && serr != WOLFSSL_ERROR_WANT_WRITE) {
          // The server gave up (e.g. after the client's alert); let the client report why.
          serverDone = true;
        }
      }
    }
  }
  if (client) wolfSSL_free(client);
  wolfSSL_free(server);
  wolfSSL_CTX_free(cctx);
  wolfSSL_CTX_free(sctx);
  close(sv[0]);
  close(sv[1]);
  return out;
}

const char kHost[] = "dogear.test";

TEST(DogearTls, TrustedCertificateForTheRightHostConnects) {
  const Outcome o = handshake(cert("good.pem"), cert("good.key"), cert("root.pem"), kHost);
  EXPECT_TRUE(o.configured);
  EXPECT_TRUE(o.connected) << "error " << o.error;
}

TEST(DogearTls, SelfSignedCertificateIsRefused) {
  const Outcome o = handshake(cert("selfsigned.pem"), cert("selfsigned.key"), cert("root.pem"), kHost);
  EXPECT_FALSE(o.connected);
  EXPECT_NE(o.error, 0);
}

TEST(DogearTls, ExpiredCertificateIsRefused) {
  const Outcome o = handshake(cert("expired.pem"), cert("expired.key"), cert("root.pem"), kHost);
  EXPECT_FALSE(o.connected);
  EXPECT_EQ(o.error, ASN_AFTER_DATE_E);
}

TEST(DogearTls, CertificateForAnotherHostIsRefused) {
  const Outcome o = handshake(cert("wronghost.pem"), cert("wronghost.key"), cert("root.pem"), kHost);
  EXPECT_FALSE(o.connected);
  EXPECT_EQ(o.error, DOMAIN_NAME_MISMATCH);
}

TEST(DogearTls, ValidCertificateDoesNotCoverADifferentRequestedHost) {
  const Outcome o = handshake(cert("good.pem"), cert("good.key"), cert("root.pem"), "evil.test");
  EXPECT_FALSE(o.connected);
  EXPECT_EQ(o.error, DOMAIN_NAME_MISMATCH);
}

TEST(DogearTls, ProductionRootsDoNotTrustTheTestRoot) {
  const std::string roots(kDogearTrustRoots);
  const Outcome o = handshake(cert("good.pem"), cert("good.key"), roots, kHost);
  EXPECT_TRUE(o.configured);  // all seven production roots load
  EXPECT_FALSE(o.connected);
}

TEST(DogearTls, FailsClosedWhenTrustCannotBeSetUp) {
  WOLFSSL_CTX* ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
  const std::string garbage = "-----BEGIN CERTIFICATE-----\nnot base64\n-----END CERTIFICATE-----\n";
  EXPECT_FALSE(dogear_tls::configureContext(ctx, garbage.data(), garbage.size()));
  EXPECT_FALSE(dogear_tls::configureContext(ctx, "", 0));
  EXPECT_FALSE(dogear_tls::configureContext(nullptr, kDogearTrustRoots, sizeof(kDogearTrustRoots) - 1));
  WOLFSSL* ssl = wolfSSL_new(ctx);
  EXPECT_FALSE(dogear_tls::configureSession(ssl, ""));
  EXPECT_FALSE(dogear_tls::configureSession(ssl, nullptr));
  wolfSSL_free(ssl);
  wolfSSL_CTX_free(ctx);
  // A failed root load still leaves peer verification on: the same handshake
  // with no usable roots never connects.
  const Outcome o = handshake(cert("good.pem"), cert("good.key"), garbage, kHost);
  EXPECT_FALSE(o.configured);
  EXPECT_FALSE(o.connected);
}

// Opt-in: DOGEAR_TLS_LIVE=1 checks the production roots against real hosts on
// the platforms DOGEAR would use (Cloudflare Pages: GTS; GitHub Pages: Let's
// Encrypt), with this wolfSSL build, including the P-384 GTS Root R4 chain.
bool liveHandshake(const char* host, const char* asHost, int& error) {
  addrinfo hints{}, *res = nullptr;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(host, "443", &hints, &res) != 0 || !res) return false;
  const int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  const bool tcp = fd >= 0 && connect(fd, res->ai_addr, res->ai_addrlen) == 0;
  freeaddrinfo(res);
  if (!tcp) return false;
  WOLFSSL_CTX* ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());
  bool ok = dogear_tls::configureContext(ctx, kDogearTrustRoots, sizeof(kDogearTrustRoots) - 1);
  WOLFSSL* ssl = wolfSSL_new(ctx);
  ok = ok && dogear_tls::configureSession(ssl, host);
  // Expect a different name than the site serves (SNI still reaches the site).
  if (ok && strcmp(host, asHost) != 0) ok = wolfSSL_check_domain_name(ssl, asHost) == WOLFSSL_SUCCESS;
  wolfSSL_set_fd(ssl, fd);
  if (ok) {
    const int rc = wolfSSL_connect(ssl);
    ok = rc == WOLFSSL_SUCCESS;
    if (!ok) error = wolfSSL_get_error(ssl, rc);
  }
  wolfSSL_free(ssl);
  wolfSSL_CTX_free(ctx);
  close(fd);
  return ok;
}

TEST(DogearTlsLive, ProductionRootsVerifyRealHosts) {
  if (!std::getenv("DOGEAR_TLS_LIVE")) GTEST_SKIP() << "set DOGEAR_TLS_LIVE=1 to run against real hosts";
  for (const char* host : {"goto.archievalmariano.com", "archievalmariano.com"}) {
    int error = 0;
    EXPECT_TRUE(liveHandshake(host, host, error)) << host << " error " << error;
  }
  int error = 0;
  EXPECT_FALSE(liveHandshake("goto.archievalmariano.com", "dogear.example.com", error));
  EXPECT_EQ(error, DOMAIN_NAME_MISMATCH);
}

// ---- the issue a verified generation must contain ----

TEST(DogearVerifiedIssue, MustBeTheIssueItsManifestNames) {
  DogearIssue builtin;
  ASSERT_TRUE(loadBuiltinDogearIssue(builtin));
  std::ifstream f(DOGEAR_BUILTIN_HEADER);
  std::stringstream ss;
  ss << f.rdbuf();
  const std::string header = ss.str();
  const size_t a = header.find("R\"DOGEARJSON(") + 13, b = header.rfind(")DOGEARJSON\"");
  ASSERT_LT(a, b);
  const std::string bytes = header.substr(a, b - a);
  DogearIssue issue;
  EXPECT_TRUE(parseVerifiedIssue(bytes, "dogear-2026-11-23", issue));
  EXPECT_EQ(issue.entries.size(), builtin.entries.size());
  EXPECT_FALSE(parseVerifiedIssue(bytes, "dogear-2026-11-30", issue));                              // wrong issue id
  EXPECT_FALSE(parseVerifiedIssue(bytes.substr(0, bytes.size() / 2), "dogear-2026-11-23", issue));  // truncated
  EXPECT_FALSE(parseVerifiedIssue("{}", "dogear-2026-11-23", issue));
}

// ---- issue schemas: 2 (regular) and 3 (a quiet week) -------------------------

std::string builtinBytes() {
  std::ifstream f(DOGEAR_BUILTIN_HEADER);
  std::stringstream ss;
  ss << f.rdbuf();
  const std::string header = ss.str();
  const size_t a = header.find("R\"DOGEARJSON(") + 13, b = header.rfind(")DOGEARJSON\"");
  return header.substr(a, b - a);
}

// A quiet issue (schema 3) built from the built-in issue: `items` entries (the
// built-in ones, repeated under new ids if needed), the first `featured` of them
// featured, the approved copy. `tweak` then breaks one thing.
std::string quietIssue(size_t items, size_t featured, const std::function<void(JsonDocument&)>& tweak = nullptr) {
  JsonDocument doc;
  if (deserializeJson(doc, builtinBytes())) return "";
  doc["schemaVersion"] = 3;
  doc["quiet"]["heading"] = dogear_limits::kQuietHeading;
  doc["quiet"]["note"] = dogear_limits::kQuietNote;
  JsonDocument source;
  source.set(doc["entries"]);
  JsonArray entries = doc["entries"].to<JsonArray>();
  const size_t have = source.as<JsonArrayConst>().size();
  for (size_t i = 0; i < items; ++i) {
    JsonObject e = entries.add<JsonObject>();
    e.set(source[i % have]);
    e["id"] = "item-" + std::to_string(i);
    e["role"] = i < featured ? "featured" : "standard";
  }
  if (tweak) tweak(doc);
  std::string out;
  serializeJson(doc, out);
  return out;
}

bool readable(const std::string& json) {
  DogearIssue issue;
  return parseDogearIssue(json.c_str(), issue);
}

TEST(DogearIssueSchema, RegularAndQuietIssuesAreRead) {
  DogearIssue issue;
  ASSERT_TRUE(parseDogearIssue(builtinBytes().c_str(), issue));  // schema 2, 6 items
  EXPECT_FALSE(issue.quiet);
  EXPECT_TRUE(issue.quietHeading.empty() && issue.quietNote.empty());
  ASSERT_TRUE(parseDogearIssue(quietIssue(4, 1).c_str(), issue));
  EXPECT_TRUE(issue.quiet);
  EXPECT_EQ(issue.quietHeading, "SOME QUIET THIS WEEK");
  EXPECT_EQ(issue.quietNote, "Not much on our calendar, but still a few things worth a dogear for you.");
  EXPECT_EQ(issue.entries.size(), 4u);
}

TEST(DogearIssueSchema, QuietCopyMustBeExactlyTheApprovedCopy) {
  const auto withCopy = [](const char* heading, const char* note) {
    return quietIssue(3, 1, [&](JsonDocument& d) {
      d["quiet"]["heading"] = heading;
      d["quiet"]["note"] = note;
    });
  };
  const char* H = dogear_limits::kQuietHeading;
  const char* N = dogear_limits::kQuietNote;
  EXPECT_TRUE(readable(withCopy(H, N)));
  const std::string maxHeading(dogear_limits::kMaxShortLen, 'H'), maxNote(dogear_limits::kMaxLineLen, 'n');
  for (const std::string& bad : {
           withCopy("SOME QUIET THIS WEEK!", N),                                                    // altered heading
           withCopy("Some quiet this week", N),                                                     // case
           withCopy("SOME QUIET THIS WEEK ", N),                                                    // trailing space
           withCopy(H, "Not much on our calendar, but still a few things worth a dogear for you"),  // altered note
           withCopy(H,
                    "Not much landed on our calendar this week. Still, we pinned a few things worth a dogear for "
                    "you."),                                                      // the superseded note
           withCopy(maxHeading.c_str(), N),                                       // within the cap, unrelated text
           withCopy(H, maxNote.c_str()),                                          // within the cap, unrelated text
           withCopy(maxHeading.c_str(), maxNote.c_str()),                         //
           quietIssue(3, 1, [](JsonDocument& d) { d.remove("quiet"); }),          // no copy
           quietIssue(3, 1, [](JsonDocument& d) { d["quiet"]["extra"] = "x"; }),  // an extra key
           quietIssue(3, 1, [](JsonDocument& d) { d["quiet"] = "SOME QUIET THIS WEEK"; }),  // not an object
       }) {
    EXPECT_FALSE(readable(bad)) << bad.substr(0, 200);
  }
  // A regular issue may not carry quiet copy at all, even the approved copy.
  JsonDocument regular;
  ASSERT_FALSE(deserializeJson(regular, builtinBytes()));
  regular["quiet"]["heading"] = H;
  regular["quiet"]["note"] = N;
  std::string withQuiet;
  serializeJson(regular, withQuiet);
  EXPECT_FALSE(readable(withQuiet));
}

TEST(DogearIssueSchema, QuietShapeIsThreeToFiveItemsExactlyOneFeaturedFirst) {
  EXPECT_TRUE(readable(quietIssue(3, 1)));
  EXPECT_TRUE(readable(quietIssue(4, 1)));
  EXPECT_TRUE(readable(quietIssue(5, 1)));
  const auto role = [](size_t i, const char* r) { return [i, r](JsonDocument& d) { d["entries"][i]["role"] = r; }; };
  struct Case {
    const char* name;
    std::string json;
  };
  const Case bad[] = {
      {"1 item", quietIssue(1, 1)},
      {"2 items", quietIssue(2, 1)},
      {"6 items", quietIssue(6, 1)},
      {"8 items", quietIssue(8, 2)},
      {"no featured item", quietIssue(4, 0)},
      {"2 featured items (3 items)", quietIssue(3, 2)},
      {"2 featured items (5 items)", quietIssue(5, 2)},
      {"3 featured items", quietIssue(4, 3)},
      {"5 featured items", quietIssue(5, 5)},
      {"unknown role", quietIssue(4, 1, role(2, "lead"))},
      {"empty role", quietIssue(4, 1, role(2, ""))},
      {"missing role", quietIssue(4, 1, [](JsonDocument& d) { d["entries"][1].remove("role"); })},
      {"featured after standard", quietIssue(4, 1,
                                             [](JsonDocument& d) {
                                               d["entries"][0]["role"] = "standard";
                                               d["entries"][2]["role"] = "featured";
                                             })},
  };
  for (const Case& c : bad) EXPECT_FALSE(readable(c.json)) << c.name;
}

TEST(DogearIssueSchema, TheContractConstants) {
  EXPECT_EQ(dogear_limits::kIssueSchemaMin, 2u);
  EXPECT_EQ(dogear_limits::kIssueSchemaMax, 3u);
  EXPECT_EQ(dogear_limits::kQuietMinEntries, 3u);
  EXPECT_EQ(dogear_limits::kQuietMaxEntries, 5u);
  EXPECT_EQ(dogear_limits::kQuietFeatured, 1u);
  for (int schema : {0, 1, 4}) {
    EXPECT_FALSE(readable(quietIssue(3, 1, [schema](JsonDocument& d) { d["schemaVersion"] = schema; })));
  }
}

// ---- a downloaded issue the firmware cannot read never becomes readable ------

class MemFs : public dogear_cache::FileOps {
 public:
  std::map<std::string, std::string> files;
  bool exists(const std::string& p) override { return files.count(p) > 0; }
  bool read(const std::string& p, std::string& out, size_t maxBytes) override {
    const auto it = files.find(p);
    if (it == files.end() || it->second.size() > maxBytes) return false;
    out = it->second;
    return true;
  }
  bool write(const std::string& p, const std::string& data) override {
    files[p] = data;
    return true;
  }
  bool rename(const std::string& from, const std::string& to) override {
    if (!files.count(from) || files.count(to)) return false;
    files[to] = files[from];
    files.erase(from);
    return true;
  }
  bool remove(const std::string& p) override {
    files.erase(p);
    return true;
  }
  bool list(const std::string& dir, std::vector<std::string>& names) override {
    for (const auto& kv : files)
      if (kv.first.rfind(dir + "/", 0) == 0) names.push_back(kv.first.substr(dir.size() + 1));
    return true;
  }
  bool ensureDir(const std::string&) override { return true; }
};

dogear_cache::Generation generation(const std::string& bytes) {
  DogearIssue issue;
  parseDogearIssue(bytes.c_str(), issue);  // only for its issue id
  JsonDocument d;
  deserializeJson(d, bytes);
  return {d["issueId"] | "", testSha256Hex(bytes), bytes};
}

TEST(DogearQuietDownload, AnUnreadableQuietIssueIsNeverCachedAndTheReadableOneStays) {
  const auto renamed = [](const char* id) { return [id](JsonDocument& d) { d["issueId"] = id; }; };
  const std::string regular = builtinBytes();  // dogear-2026-11-23, schema 2
  const std::string badCopy = quietIssue(4, 1, [&](JsonDocument& d) {
    renamed("dogear-2026-11-30")(d);
    d["quiet"]["note"] = "Something else entirely.";
  });
  const std::string badShape = quietIssue(6, 1, renamed("dogear-2026-12-07"));
  const std::string twoFeatured = quietIssue(5, 2, renamed("dogear-2026-12-14"));
  const std::string good = quietIssue(5, 1, renamed("dogear-2026-12-21"));
  const auto parse = [](const dogear_cache::Generation& g) {
    DogearIssue issue;
    return parseVerifiedIssue(g.bytes, g.issueId, issue);
  };

  MemFs fs;
  dogear_cache::Cache cache(fs, testSha256Hex);
  ASSERT_TRUE(cache.commit("dogear-2026-11-23", testSha256Hex(regular), regular));
  const auto before = fs.files;
  for (const std::string* malformed : {&badCopy, &badShape, &twoFeatured}) {
    EXPECT_EQ(dogear_cache::admitDownload(cache, generation(*malformed), parse), dogear_cache::Admitted::Unreadable);
    EXPECT_EQ(fs.files, before) << "nothing was written for an unreadable issue";
    dogear_cache::Generation shown;
    EXPECT_STREQ(dogear_cache::loadNewestReadable(cache, shown, parse), dogear_cache::kActive);
    EXPECT_EQ(shown.issueId, "dogear-2026-11-23");  // the readable issue stays the one shown
  }
  // A well-formed quiet issue is admitted and becomes the newest readable generation.
  EXPECT_EQ(dogear_cache::admitDownload(cache, generation(good), parse), dogear_cache::Admitted::Cached);
  dogear_cache::Generation shown;
  ASSERT_STREQ(dogear_cache::loadNewestReadable(cache, shown, parse), dogear_cache::kActive);
  EXPECT_EQ(shown.issueId, "dogear-2026-12-21");
  ASSERT_TRUE(cache.loadFrom(dogear_cache::kPrevious, shown));
  EXPECT_EQ(shown.issueId, "dogear-2026-11-23");
}

}  // namespace
