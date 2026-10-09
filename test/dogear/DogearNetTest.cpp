#include <gtest/gtest.h>

#include "activities/dogear/DogearHttpParser.h"
#include "activities/dogear/DogearNet.h"

namespace {
using dogear_net::Policy;
using dogear_net::Url;

Url url(const std::string& s) {
  Url u;
  EXPECT_TRUE(dogear_net::parseUrl(s, u)) << s;
  return u;
}

TEST(DogearNet, ParsesStrictAbsoluteUrls) {
  Url u = url("https://Dogear.ArchievalMariano.com/issues/dogear-2026-11-23.json");
  EXPECT_TRUE(u.https);
  EXPECT_EQ(u.host, "dogear.archievalmariano.com");
  EXPECT_EQ(u.port, 443);
  EXPECT_EQ(u.path, "/issues/dogear-2026-11-23.json");
  u = url("http://192.168.1.50:8080");
  EXPECT_FALSE(u.https);
  EXPECT_EQ(u.port, 8080);
  EXPECT_EQ(u.path, "/");
  Url bad;
  for (const char* s : {"https://", "https:///x", "ftp://x.org/", "https://user@x.org/", "https://x .org/",
                        "https://x.org:0/", "https://x.org:65536/", "https://-x.org/", "https://x..org/",
                        "https://x.org:/", "//x.org/", "https://x_y.org/"}) {
    EXPECT_FALSE(dogear_net::parseUrl(s, bad)) << s;
  }
}

TEST(DogearNet, PlainHttpOnlyForPrivateLanInDevBuilds) {
  EXPECT_TRUE(dogear_net::urlAllowed(url("https://dogear.archievalmariano.com/"), Policy::HttpsOnly));
  EXPECT_FALSE(dogear_net::urlAllowed(url("http://192.168.1.50:8080/"), Policy::HttpsOnly));
  EXPECT_TRUE(dogear_net::urlAllowed(url("http://192.168.1.50:8080/"), Policy::AllowLanHttp));
  EXPECT_TRUE(dogear_net::urlAllowed(url("http://10.0.0.2/"), Policy::AllowLanHttp));
  EXPECT_TRUE(dogear_net::urlAllowed(url("http://172.20.1.1/"), Policy::AllowLanHttp));
  EXPECT_TRUE(dogear_net::urlAllowed(url("http://mac.local/"), Policy::AllowLanHttp));
  // The exception never covers public hosts or near-miss addresses.
  for (const char* s : {"http://dogear.archievalmariano.com/", "http://8.8.8.8/", "http://172.32.0.1/",
                        "http://192.169.0.1/", "http://local/", "http://192.168.1/"}) {
    EXPECT_FALSE(dogear_net::urlAllowed(url(s), Policy::AllowLanHttp)) << s;
  }
}

TEST(DogearNet, RedirectsNeverDowngradeHttps) {
  const Url from = url("https://dogear.archievalmariano.com/current.json");
  Url to;
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "https://cdn.example.org/current.json", to));
  EXPECT_TRUE(dogear_net::redirectAllowed(from, to, Policy::HttpsOnly));  // valid HTTPS redirect
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "/2026/current.json", to));
  EXPECT_EQ(to.host, "dogear.archievalmariano.com");
  EXPECT_TRUE(to.https);
  EXPECT_TRUE(dogear_net::redirectAllowed(from, to, Policy::HttpsOnly));
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "http://dogear.archievalmariano.com/current.json", to));
  EXPECT_FALSE(dogear_net::redirectAllowed(from, to, Policy::HttpsOnly));  // HTTPS -> HTTP
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "http://192.168.1.2/current.json", to));
  EXPECT_FALSE(dogear_net::redirectAllowed(from, to, Policy::AllowLanHttp));  // not even to the LAN
  // Relative forms DOGEAR does not need are refused outright.
  EXPECT_FALSE(dogear_net::resolveRedirect(from, "//evil.example/x", to));
  EXPECT_FALSE(dogear_net::resolveRedirect(from, "current.json", to));
  EXPECT_FALSE(dogear_net::resolveRedirect(from, "", to));
}

TEST(DogearNet, LanHttpMayUpgradeButNotLeaveTheLanInClear) {
  const Url from = url("http://192.168.1.50:8080/current.json");
  Url to;
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "/issues/x.json", to));
  EXPECT_EQ(to.port, 8080);
  EXPECT_TRUE(dogear_net::redirectAllowed(from, to, Policy::AllowLanHttp));
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "http://example.org/x", to));
  EXPECT_FALSE(dogear_net::redirectAllowed(from, to, Policy::AllowLanHttp));
  ASSERT_TRUE(dogear_net::resolveRedirect(from, "https://example.org/x", to));
  EXPECT_TRUE(dogear_net::redirectAllowed(from, to, Policy::AllowLanHttp));
}

TEST(DogearNet, ClockMustBeSetBeforeCertificatesAreChecked) {
  EXPECT_FALSE(dogear_net::clockPlausible(0));           // 1970: never set
  EXPECT_FALSE(dogear_net::clockPlausible(1767225599));  // 2025-12-31
  EXPECT_TRUE(dogear_net::clockPlausible(1791331200));   // 2026-10-07
}

// ---- response parser ----

using dogear_http::ResponseParser;

TEST(DogearHttp, ContentLengthBody) {
  ResponseParser p(64);
  const std::string r = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nContent-Type: application/json\r\n\r\nhello";
  for (char c : r) ASSERT_TRUE(p.feed(&c, 1));  // byte by byte
  EXPECT_TRUE(p.done());
  EXPECT_EQ(p.status(), 200);
  EXPECT_EQ(p.body(), "hello");
}

TEST(DogearHttp, ChunkedBodyWithExtensionsAndTrailers) {
  ResponseParser p(64);
  const std::string r =
      "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n4;x=y\r\nhell\r\n1\r\no\r\n0\r\nX-Trailer: 1\r\n\r\n";
  ASSERT_TRUE(p.feed(r.data(), r.size()));
  EXPECT_TRUE(p.done());
  EXPECT_EQ(p.body(), "hello");
}

TEST(DogearHttp, CloseDelimitedBodyEndsOnlyAtEof) {
  ResponseParser p(64);
  const std::string r = "HTTP/1.0 200 OK\r\n\r\nabc";
  ASSERT_TRUE(p.feed(r.data(), r.size()));
  EXPECT_FALSE(p.done());
  EXPECT_TRUE(p.finishAtEof());
  EXPECT_EQ(p.body(), "abc");
}

TEST(DogearHttp, TruncatedBodiesAreIncomplete) {
  ResponseParser a(64);
  const std::string r1 = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nabc";
  ASSERT_TRUE(a.feed(r1.data(), r1.size()));
  EXPECT_FALSE(a.finishAtEof());
  ResponseParser b(64);
  const std::string r2 = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nab";
  ASSERT_TRUE(b.feed(r2.data(), r2.size()));
  EXPECT_FALSE(b.finishAtEof());
}

TEST(DogearHttp, CapsAndMalformedResponsesFail) {
  ResponseParser declared(4);
  const std::string big = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n";
  EXPECT_FALSE(declared.feed(big.data(), big.size()));  // declared over the cap
  ResponseParser streamed(4);
  const std::string close = "HTTP/1.1 200 OK\r\n\r\nhello";
  EXPECT_FALSE(streamed.feed(close.data(), close.size()));  // undeclared, over the cap
  ResponseParser chunk(4);
  const std::string c = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n10\r\n";
  EXPECT_FALSE(chunk.feed(c.data(), c.size()));
  for (const char* bad :
       {"HTTP/2 200 OK\r\n\r\n", "HTTP/1.1 2x0 OK\r\n\r\n", "HTTP/1.1 200 OK\r\nNoColon\r\n\r\n",
        "HTTP/1.1 200 OK\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\n",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n", "HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\n"}) {
    ResponseParser p(64);
    EXPECT_FALSE(p.feed(bad, strlen(bad))) << bad;
  }
}

TEST(DogearHttp, RedirectsExposeTheirLocation) {
  ResponseParser p(64);
  const std::string r = "HTTP/1.1 301 Moved\r\nLocation:  https://x.org/a \r\nContent-Length: 0\r\n\r\n";
  ASSERT_TRUE(p.feed(r.data(), r.size()));
  EXPECT_TRUE(p.isRedirect());
  EXPECT_EQ(p.location(), "https://x.org/a");
}

}  // namespace
