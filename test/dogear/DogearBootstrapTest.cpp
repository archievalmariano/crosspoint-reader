// "Nothing published": the only server answer that may put NO ISSUE YET on
// screen. dogear_http::readResponse and classify over the issue server's own
// router vectors (vendored in fixtures/RouterVectors.inc), near misses,
// truncation, framing (explicit framing only), read failures and the non-200
// body cap.
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "activities/dogear/DogearHttpParser.h"

namespace {
using dogear_http::classify;
using dogear_http::kReadClosed;
using dogear_http::kReadFailed;
using dogear_http::Outcome;
using dogear_http::ResponseParser;
using dogear_http::StreamEnd;

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

// The transport delivers `bytes` in `chunk`-byte reads, then `last`
// (kReadClosed: a clean end; kReadFailed: a read error). dogear_https::get
// reads and classifies exactly like this.
Outcome run(const std::string& bytes, bool firstManifestResponse = true, int last = kReadClosed, size_t chunk = 7) {
  ResponseParser p(kManifestCap);
  size_t at = 0;
  const StreamEnd end = dogear_http::readResponse(
      p,
      [&](char* buf, size_t len) {
        if (at >= bytes.size()) return last;
        const size_t n = std::min({len, chunk, bytes.size() - at});
        bytes.copy(buf, n, at);
        at += n;
        return static_cast<int>(n);
      },
      [] { return false; });
  return classify(p, end, firstManifestResponse);
}

const Framing kFramings[] = {Framing::Length, Framing::Chunked, Framing::Close};
const Framing kExplicit[] = {Framing::Length, Framing::Chunked};

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
      // NothingPublished needs explicit framing: a close-delimited "empty" is an error.
      const Outcome framed = want == Outcome::NothingPublished && f == Framing::Close ? Outcome::Error : want;
      EXPECT_EQ(run(bytes), framed) << store << " framing " << static_cast<int>(f);
      // A read failure where the clean end would be: an explicitly framed response
      // is complete before it; a close-delimited one never completes.
      EXPECT_EQ(run(bytes, true, kReadFailed), f == Framing::Close ? Outcome::Error : framed)
          << store << " framing " << static_cast<int>(f);
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
  for (const Framing f : kExplicit) EXPECT_EQ(run(response(503, kRight, f)), Outcome::NothingPublished);
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

TEST(DogearBootstrap, OnlyExplicitFramingCounts) {
  // Close-delimited: the body ends only where the connection ends, which a cut
  // connection can fake. Never NothingPublished, whether it closed cleanly or not.
  const std::string closeDelimited = response(503, kRight, Framing::Close);
  EXPECT_EQ(run(closeDelimited, true, kReadClosed), Outcome::Error);
  EXPECT_EQ(run(closeDelimited, true, kReadFailed), Outcome::Error);
  EXPECT_EQ(run(response(503, kRight, Framing::Length)), Outcome::NothingPublished);
  EXPECT_EQ(run(response(503, kRight, Framing::Chunked)), Outcome::NothingPublished);
  // A close-delimited 200 is still a body, as before (issue downloads).
  EXPECT_EQ(run(response(200, "{}", Framing::Close)), Outcome::Body);
  EXPECT_EQ(run(response(200, "{}", Framing::Close), true, kReadFailed), Outcome::Error);
}

TEST(DogearBootstrap, ATlsReadFailureAfterTheExactBodyIsNeverNothingPublished) {
  // Headers with no framing, the exact 18-byte body, then the read fails (a TLS
  // error, or the connection cut without close_notify).
  const std::string exact = statusLine(503) + "\r\n" + kRight;
  for (const size_t chunk : {size_t{1}, size_t{7}, exact.size()}) {
    EXPECT_EQ(run(exact, true, kReadFailed, chunk), Outcome::Error) << chunk;
    EXPECT_EQ(run(exact, true, kReadClosed, chunk), Outcome::Error) << chunk;
  }
  // A failure before an explicitly framed response is complete: never complete.
  for (const Framing f : kFramings) {
    const std::string full = response(503, kRight, f);
    for (size_t cut = 0; cut < full.size(); ++cut) {
      EXPECT_EQ(run(full.substr(0, cut), true, kReadFailed), Outcome::Error)
          << "framing " << static_cast<int>(f) << " cut at " << cut;
    }
  }
  // classify itself: a failed read is an error, whatever the parser holds.
  for (const Framing f : kFramings) {
    ResponseParser p(kManifestCap);
    const std::string full = response(503, kRight, f);
    p.feed(full.data(), full.size());
    EXPECT_EQ(classify(p, StreamEnd::Failed, true), Outcome::Error) << static_cast<int>(f);
  }
}

TEST(DogearBootstrap, TheReaderStopsAtCompletionAndReportsHowItEnded) {
  // Explicit framing: the reader stops once the response is complete and never
  // reads what follows.
  const std::string full = response(503, kRight, Framing::Length);
  int reads = 0;
  ResponseParser p(kManifestCap);
  const StreamEnd end = dogear_http::readResponse(
      p,
      [&](char* buf, size_t) {
        if (reads++ > 0) return kReadFailed;
        full.copy(buf, full.size());
        return static_cast<int>(full.size());
      },
      [] { return false; });
  EXPECT_EQ(end, StreamEnd::Open);
  EXPECT_EQ(reads, 1);
  EXPECT_EQ(classify(p, end, true), Outcome::NothingPublished);
  // How it ended, as reported.
  ResponseParser c(kManifestCap), f(kManifestCap), w(kManifestCap);
  EXPECT_EQ(
      dogear_http::readResponse(c, [](char*, size_t) { return kReadClosed; }, [] { return true; }), StreamEnd::Closed);
  EXPECT_EQ(
      dogear_http::readResponse(f, [](char*, size_t) { return kReadFailed; }, [] { return true; }), StreamEnd::Failed);
  int idles = 0;
  EXPECT_EQ(
      dogear_http::readResponse(w, [](char*, size_t) { return 0; }, [&] { return ++idles < 5; }), StreamEnd::Open);
  EXPECT_EQ(idles, 5);
  EXPECT_EQ(classify(w, StreamEnd::Open, true), Outcome::Error);  // timed out: incomplete
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
    for (const int last : {kReadClosed, kReadFailed}) EXPECT_EQ(run(bytes, true, last), Outcome::Error) << length;
  }
}

TEST(DogearBootstrap, EveryTruncationIsAnError) {
  for (const Framing f : kFramings) {
    const std::string full = response(503, kRight, f);
    for (size_t n = 0; n < full.size(); ++n) {
      for (const int last : {kReadClosed, kReadFailed}) {
        EXPECT_NE(run(full.substr(0, n), true, last), Outcome::NothingPublished)
            << "framing " << static_cast<int>(f) << " cut at " << n;
      }
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
      EXPECT_TRUE(p.failed()) << body->size() << " bytes, framing " << static_cast<int>(f);
      EXPECT_EQ(classify(p, StreamEnd::Closed, true), Outcome::Error);
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

TEST(DogearBootstrap, BytesReadOneAtATimeClassifyTheSame) {
  for (const Framing f : kExplicit) {
    EXPECT_EQ(run(response(503, kRight, f), true, kReadClosed, 1), Outcome::NothingPublished);
    EXPECT_EQ(run(response(503, kRight, f), true, kReadFailed, 1), Outcome::NothingPublished);
  }
  EXPECT_EQ(run(response(503, kRight, Framing::Close), true, kReadClosed, 1), Outcome::Error);
}

}  // namespace
