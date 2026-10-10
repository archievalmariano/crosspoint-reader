// "Nothing published": the only server answer that may put NO ISSUE YET on
// screen. dogear_http::classify over the issue server's own router vectors
// (vendored in fixtures/RouterVectors.inc), near misses, truncation, framing
// and the non-200 body cap.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "activities/dogear/DogearHttpParser.h"

namespace {
using dogear_http::classify;
using dogear_http::Outcome;
using dogear_http::ResponseParser;

constexpr size_t kManifestCap = 4096;  // as dogear_limits::kMaxManifestBytes order of size
const std::string kRight = "nothing published\n";

struct Vector {
  const char* store;
  int status;
  const char* headers;
  const char* body;
};
const Vector kVectors[] = {
#include "fixtures/RouterVectors.inc"
};

enum class Framing { Length, Chunked, Close };

std::string statusLine(int status) { return "HTTP/1.1 " + std::to_string(status) + " X\r\n"; }

std::string response(int status, const std::string& body, Framing framing, const std::string& headers = "") {
  std::string r = statusLine(status) + headers;
  switch (framing) {
    case Framing::Length:
      return r + "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    case Framing::Chunked: {
      char size[16];
      snprintf(size, sizeof(size), "%zx", body.size());
      r += "Transfer-Encoding: chunked\r\n\r\n";
      if (!body.empty()) r += std::string(size) + "\r\n" + body + "\r\n";
      return r + "0\r\n\r\n";
    }
    case Framing::Close:
      return r + "\r\n" + body;
  }
  return r;
}

// Feed `bytes` (then a connection close) and classify, as dogear_https::get does.
Outcome run(const std::string& bytes, bool firstManifestResponse = true, size_t cap = kManifestCap) {
  ResponseParser p(cap);
  p.feed(bytes.data(), bytes.size());
  p.finishAtEof();
  return classify(p, firstManifestResponse);
}

const Framing kFramings[] = {Framing::Length, Framing::Chunked, Framing::Close};

TEST(DogearBootstrap, RouterVectorsClassifyExactlyAsTheServerMeansThem) {
  ASSERT_EQ(sizeof(kVectors) / sizeof(kVectors[0]), 31u);
  int nothing = 0, body = 0, error = 0;
  for (const Vector& v : kVectors) {
    const std::string store = v.store;
    const Outcome want = store == "empty"                            ? Outcome::NothingPublished
                         : (store == "published" || store == "held") ? Outcome::Body
                                                                     : Outcome::Error;
    for (const Framing f : kFramings) {
      const std::string bytes = response(v.status, v.body, f, v.headers);
      EXPECT_EQ(run(bytes), want) << store << " framing " << static_cast<int>(f);
      // Never on a later response (after a redirect) or for an issue download.
      EXPECT_NE(run(bytes, false), Outcome::NothingPublished) << store;
    }
    nothing += want == Outcome::NothingPublished;
    body += want == Outcome::Body;
    error += want == Outcome::Error;
  }
  EXPECT_EQ(nothing, 1);  // "empty"
  EXPECT_EQ(body, 2);     // "published", "held"
  EXPECT_EQ(error, 28);   // every corrupt:* registry: 503 "unavailable"
}

TEST(DogearBootstrap, OnlyTheExactBodyCounts) {
  for (const Framing f : kFramings) EXPECT_EQ(run(response(503, kRight, f)), Outcome::NothingPublished);
  const std::string nearMisses[] = {
      "nothing published",      // no newline
      "nothing published\r\n",  // CRLF
      "nothing published\n\n",  // extra newline
      "nothing published\nx",   // trailing byte
      "Nothing published\n",    // case
      "nothing  published\n",   // spacing
      " nothing published\n",   // leading space
      "nothing published \n",   // trailing space
      "nothing_published\n",    //
      "unavailable\n",          // the storage-error answer
      "",                       // empty
      std::string("nothing published\n\0", 19),
      std::string("nothing\0published\n", 18),
  };
  for (const std::string& b : nearMisses) {
    for (const Framing f : kFramings) EXPECT_EQ(run(response(503, b, f)), Outcome::Error) << "[" << b << "]";
  }
}

TEST(DogearBootstrap, OnlyStatus503) {
  for (const int status : {500, 502, 504, 404, 403, 429, 501, 418}) {
    for (const Framing f : kFramings) EXPECT_EQ(run(response(status, kRight, f)), Outcome::Error) << status;
  }
  // A 200 with that body is a (malformed) manifest body, never NothingPublished.
  EXPECT_EQ(run(response(200, kRight, Framing::Length)), Outcome::Body);
}

TEST(DogearBootstrap, OnlyTheFirstManifestResponse) {
  for (const Framing f : kFramings) EXPECT_EQ(run(response(503, kRight, f), false), Outcome::Error);
  // The redirect itself is followed; the 503 after it is an error (run(..., false)).
  const std::string redirect =
      statusLine(302) + "Location: https://dogear.archievalmariano.com/x\r\nContent-Length: 0\r\n\r\n";
  EXPECT_EQ(run(redirect), Outcome::Redirect);
}

TEST(DogearBootstrap, ContentLengthMustMatchTheBody) {
  for (const size_t length : {17u, 19u, 0u, 1u}) {
    const std::string bytes = statusLine(503) + "Content-Length: " + std::to_string(length) + "\r\n\r\n" + kRight;
    EXPECT_EQ(run(bytes), Outcome::Error) << length;
  }
}

TEST(DogearBootstrap, EveryTruncationIsAnError) {
  for (const Framing f : kFramings) {
    const std::string full = response(503, kRight, f);
    for (size_t n = 0; n < full.size(); ++n) {
      const Outcome got = run(full.substr(0, n));
      // A close-delimited body ends at EOF, so only its full length can match.
      EXPECT_NE(got, Outcome::NothingPublished) << "framing " << static_cast<int>(f) << " cut at " << n;
    }
  }
}

TEST(DogearBootstrap, NonOkBodiesAreCappedAt64Bytes) {
  const std::string at64(64, 'x'), over(65, 'x'), huge(1 << 20, 'x');
  for (const Framing f : kFramings) {
    ResponseParser ok(kManifestCap);
    const std::string r64 = response(503, at64, f);
    ok.feed(r64.data(), r64.size());
    ok.finishAtEof();
    EXPECT_TRUE(ok.done()) << "64 bytes fit, framing " << static_cast<int>(f);
    for (const std::string* body : {&over, &huge}) {
      ResponseParser p(kManifestCap);
      const std::string r = response(503, *body, f);
      p.feed(r.data(), r.size());
      p.finishAtEof();
      EXPECT_TRUE(p.failed()) << body->size() << " bytes, framing " << static_cast<int>(f);
      EXPECT_EQ(classify(p, true), Outcome::Error);
    }
  }
  // The cap applies to every status but 200 and redirects.
  const std::string page(1000, 'x');
  EXPECT_EQ(run(response(200, page, Framing::Length)), Outcome::Body);
  const std::string redirect = statusLine(301) + "Location: /x\r\nContent-Length: 1000\r\n\r\n" + page;
  EXPECT_EQ(run(redirect), Outcome::Redirect);
  EXPECT_EQ(run(response(404, page, Framing::Length)), Outcome::Error);
}

TEST(DogearBootstrap, ChunkedFraming) {
  EXPECT_EQ(run(response(503, kRight, Framing::Chunked)), Outcome::NothingPublished);
  // The body in two chunks is the same body.
  const std::string split =
      statusLine(503) + "Transfer-Encoding: chunked\r\n\r\n8\r\nnothing \r\na\r\npublished\n\r\n0\r\n\r\n";
  EXPECT_EQ(run(split), Outcome::NothingPublished);
  const std::string malformed[] = {
      statusLine(503) + "Transfer-Encoding: chunked\r\n\r\n12\r\nnothing published\nX0\r\n\r\n",  // no CRLF after data
      statusLine(503) + "Transfer-Encoding: chunked\r\n\r\nzz\r\nnothing published\n\r\n0\r\n\r\n",  // bad size
      statusLine(503) + "Transfer-Encoding: chunked\r\n\r\n12\r\nnothing published\n\r\n",           // no last chunk
      statusLine(503) + "Transfer-Encoding: gzip\r\n\r\nnothing published\n",                        // other coding
      statusLine(503) + "Transfer-Encoding: chunked\r\n\r\n41\r\n" + std::string(65, 'x') + "\r\n0\r\n\r\n",
  };
  for (const std::string& m : malformed) EXPECT_EQ(run(m), Outcome::Error) << m;
}

TEST(DogearBootstrap, BytesFedOneAtATimeClassifyTheSame) {
  for (const Framing f : kFramings) {
    const std::string full = response(503, kRight, f);
    ResponseParser p(kManifestCap);
    for (const char c : full) p.feed(&c, 1);
    p.finishAtEof();
    EXPECT_EQ(classify(p, true), Outcome::NothingPublished);
  }
}

}  // namespace
