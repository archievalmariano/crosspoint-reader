#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

// A small push parser for one HTTP/1.1 response (pure, host-testable). DOGEAR
// sends "Connection: close" GETs and reads a status line, headers, and a body
// framed by Content-Length, chunked encoding, or connection close. The body is
// collected into `body()` and capped: a response over the cap is an error, so
// an oversized issue never reaches the parser or the SD card. A status other
// than 200 (or a redirect) keeps at most kMaxStatusBodyBytes of body.
namespace dogear_http {

// The body cap for any status but 200 and redirects: enough for the server's
// short plain-text answers, never a page of HTML.
inline constexpr size_t kMaxStatusBodyBytes = 64;

// The issue server's answer to /current.json while nothing has been published
// (503, exactly these 18 bytes). Any other 503 body means it is unavailable.
inline constexpr char kNothingPublishedBody[] = "nothing published\n";

class ResponseParser {
 public:
  enum class State : uint8_t { StatusLine, Headers, Body, ChunkSize, ChunkData, ChunkDataEnd, Trailers, Done, Error };

  explicit ResponseParser(size_t maxBody) : maxBody(maxBody) {}

  // Feed received bytes. False once the response is malformed or too large.
  bool feed(const char* data, size_t len) {
    for (size_t i = 0; i < len && state != State::Error && state != State::Done; ++i) step(data[i]);
    return state != State::Error;
  }

  // The connection closed. True if the response is complete: a close only
  // ends a body that had no Content-Length and was not chunked.
  bool finishAtEof() {
    if (state == State::Body && !hasLength && !chunked) state = State::Done;
    return state == State::Done;
  }

  bool headersDone() const { return state != State::StatusLine && state != State::Headers && state != State::Error; }
  // The body's end is stated by the response itself (Content-Length or chunked),
  // not inferred from the connection closing.
  bool explicitlyFramed() const { return hasLength || chunked; }
  bool done() const { return state == State::Done; }
  bool failed() const { return state == State::Error; }
  int status() const { return statusCode; }
  const std::string& location() const { return locationHeader; }
  const std::string& body() const { return bodyBytes; }
  bool isRedirect() const {
    return statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308;
  }

 private:
  static constexpr size_t kMaxLine = 2048;     // status line, one header, a chunk-size line
  static constexpr size_t kMaxHeaders = 8192;  // all header bytes together

  size_t maxBody;
  State state = State::StatusLine;
  std::string line;
  size_t headerBytes = 0;
  int statusCode = 0;
  bool chunked = false;
  bool hasLength = false;
  size_t contentLength = 0;
  size_t chunkLeft = 0;
  std::string locationHeader;
  std::string bodyBytes;

  void fail() { state = State::Error; }

  static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    return s;
  }
  static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) --b;
    return s.substr(a, b - a);
  }
  static bool parseDecimal(const std::string& s, size_t& out) {
    if (s.empty() || s.size() > 10) return false;
    out = 0;
    for (const char c : s) {
      if (c < '0' || c > '9') return false;
      out = out * 10 + static_cast<size_t>(c - '0');
    }
    return true;
  }

  void appendBody(char c) {
    if (bodyBytes.size() >= maxBody) return fail();
    bodyBytes.push_back(c);
  }

  // Collect a CRLF-terminated line; returns true when `line` is complete.
  bool takeLine(char c) {
    if (c == '\n') {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      return true;
    }
    if (line.size() >= kMaxLine) {
      fail();
      return false;
    }
    line.push_back(c);
    return false;
  }

  void onStatusLine() {
    // "HTTP/1.1 200 OK"
    if (line.rfind("HTTP/1.", 0) != 0 || line.size() < 12 || line[8] != ' ') return fail();
    size_t code;
    if (!parseDecimal(line.substr(9, 3), code) || code < 100 || code > 599) return fail();
    if (line.size() > 12 && line[12] != ' ') return fail();
    statusCode = static_cast<int>(code);
    if (statusCode != 200 && !isRedirect() && maxBody > kMaxStatusBodyBytes) maxBody = kMaxStatusBodyBytes;
    state = State::Headers;
  }

  void onHeaderLine() {
    if (line.empty()) {
      if (chunked) {
        state = State::ChunkSize;
      } else if (hasLength) {
        if (contentLength > maxBody) return fail();
        state = contentLength == 0 ? State::Done : State::Body;
      } else {
        state = State::Body;
      }
      return;
    }
    const size_t colon = line.find(':');
    if (colon == std::string::npos || colon == 0) return fail();
    const std::string name = lower(trim(line.substr(0, colon)));
    const std::string value = trim(line.substr(colon + 1));
    if (name == "content-length") {
      size_t n;
      if (!parseDecimal(value, n)) return fail();
      if (hasLength && n != contentLength) return fail();  // conflicting lengths
      hasLength = true;
      contentLength = n;
    } else if (name == "transfer-encoding") {
      const std::string v = lower(value);
      if (v != "chunked") return fail();  // no other coding is accepted
      chunked = true;
    } else if (name == "location") {
      locationHeader = value;
    }
  }

  void onChunkSizeLine() {
    const size_t semi = line.find(';');  // ignore chunk extensions
    const std::string hex = trim(semi == std::string::npos ? line : line.substr(0, semi));
    if (hex.empty() || hex.size() > 8) return fail();
    size_t n = 0;
    for (const char c : hex) {
      int d;
      if (c >= '0' && c <= '9')
        d = c - '0';
      else if (c >= 'a' && c <= 'f')
        d = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F')
        d = c - 'A' + 10;
      else
        return fail();
      n = n * 16 + static_cast<size_t>(d);
    }
    if (n > maxBody) return fail();
    chunkLeft = n;
    state = n == 0 ? State::Trailers : State::ChunkData;
  }

  void step(char c) {
    switch (state) {
      case State::StatusLine:
      case State::Headers:
      case State::ChunkSize:
      case State::Trailers: {
        if (state == State::StatusLine || state == State::Headers) {
          if (++headerBytes > kMaxHeaders) return fail();
        }
        if (!takeLine(c)) return;
        const State was = state;
        if (was == State::StatusLine)
          onStatusLine();
        else if (was == State::Headers)
          onHeaderLine();
        else if (was == State::ChunkSize)
          onChunkSizeLine();
        else if (line.empty())
          state = State::Done;  // end of trailers
        line.clear();
        return;
      }
      case State::Body:
        appendBody(c);
        if (state != State::Error && hasLength && bodyBytes.size() == contentLength) state = State::Done;
        return;
      case State::ChunkData:
        appendBody(c);
        if (state != State::Error && --chunkLeft == 0) state = State::ChunkDataEnd;
        return;
      case State::ChunkDataEnd:
        // Expect CRLF after each chunk's data.
        if (c == '\r' && line.empty()) {
          line.push_back(c);
        } else if (c == '\n') {
          line.clear();
          state = State::ChunkSize;
        } else {
          fail();
        }
        return;
      case State::Done:
      case State::Error:
        return;
    }
  }
};

enum class Outcome : uint8_t { Body, NothingPublished, Redirect, Error };

// How reading one response stopped.
//   Open   the reader stopped first: the response was complete, a redirect's
//          headers arrived, or the wait for data ran out.
//   Closed the peer ended the stream cleanly (TCP close; TLS close_notify).
//   Failed a read error (a TLS failure, a connection cut without close_notify):
//          never the end of a response.
enum class StreamEnd : uint8_t { Open, Closed, Failed };

// What a transport's receive returns besides a byte count (>0) or 0 (nothing yet).
inline constexpr int kReadClosed = -1;
inline constexpr int kReadFailed = -2;

// Read one response into `p`. `receive(buf, len)` returns bytes read (>0), 0
// for nothing yet, kReadClosed or kReadFailed. `idle()` waits briefly and
// returns false once the wait for data has run out. A redirect needs only its
// headers.
template <typename Receive, typename Idle>
StreamEnd readResponse(ResponseParser& p, Receive&& receive, Idle&& idle) {
  char buf[256];  // the stack-buffer guideline
  while (!p.done() && !p.failed()) {
    if (p.headersDone() && p.isRedirect()) break;
    const int n = receive(buf, sizeof(buf));
    if (n > 0) {
      if (!p.feed(buf, static_cast<size_t>(n))) break;
    } else if (n == kReadClosed) {
      return StreamEnd::Closed;
    } else if (n == kReadFailed) {
      return StreamEnd::Failed;
    } else if (!idle()) {
      break;
    }
  }
  return StreamEnd::Open;
}

// What one response means. `firstManifestResponse`: the response to the first
// request of a /current.json fetch (no redirect followed). Only that response,
// read without error, complete by its own explicit framing (Content-Length or
// chunked, never a connection close), with status 503 and exactly
// kNothingPublishedBody, says "nothing has been published yet"; everything else
// that is not a complete 200 (or a redirect to follow) is an error.
inline Outcome classify(ResponseParser& p, StreamEnd end, bool firstManifestResponse) {
  if (end == StreamEnd::Failed) return Outcome::Error;  // a read failure never completes a response
  if (end == StreamEnd::Closed) p.finishAtEof();        // a close ends only a body framed by the close
  if (p.failed() || !p.headersDone()) return Outcome::Error;
  if (p.isRedirect()) return Outcome::Redirect;
  if (!p.done()) return Outcome::Error;
  if (p.status() == 200) return Outcome::Body;
  if (firstManifestResponse && p.explicitlyFramed() && p.status() == 503 &&
      p.body() == std::string(kNothingPublishedBody)) {
    return Outcome::NothingPublished;
  }
  return Outcome::Error;
}

}  // namespace dogear_http
