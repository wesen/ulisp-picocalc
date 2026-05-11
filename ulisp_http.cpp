// ulisp_http.cpp — cooperative HTTP/1.x POC primitives for Pico 2W.
//
// The old Arduino-Pico WiFiServer/WiFiClient transport is intentionally
// compiled out while the Wi-Fi stack is being ported to direct CYW43/lwIP.
// The bounded parser below is kept for the upcoming raw-lwIP HTTP transport.

#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ulisp_config.h"


#include "ulisp_types.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_runtime.h"
#include "ulisp_print.h"
#include "ulisp_http.h"

namespace {

constexpr uint16_t kHttpPort = 80;
constexpr size_t kRequestBufferSize = 1024;
constexpr size_t kMaxHeaderLine = 192;
constexpr size_t kWriteChunkSize = 64;
constexpr unsigned long kParseBudgetMs = 25;
constexpr unsigned long kClientTimeoutMs = 2000;

#if defined(ULISP_WIFI) && defined(ULISP_HTTP_ARDUINO_TRANSPORT)
WiFiServer HttpServer(kHttpPort);
WiFiClient HttpClient;
bool HttpServerEnabled = false;
bool HttpClientActive = false;
bool HttpAwaitingResponse = false;
bool HttpRequestIsHead = false;
char HttpRequestBuffer[kRequestBufferSize + 1];
size_t HttpRequestLength = 0;
unsigned long HttpClientAcceptedAt = 0;
#endif

object *make_string(const char *s) {
  return lispstring((char *)s);
}

object *make_pair(const char *key, object *value) {
  return cons(make_string(key), value);
}

object *alist_put(object *alist, const char *key, object *value) {
  return cons(make_pair(key, value), alist);
}

object *make_string_range(const char *start, size_t len) {
  object *obj = newstring();
  object *tail = obj;
  for (size_t i = 0; i < len; i++) buildstring(start[i], &tail);
  return obj;
}

void trim_in_place(char *&start) {
  while (*start == ' ' || *start == '\t') start++;
  char *end = start + strlen(start);
  while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
    *--end = '\0';
  }
}

void lower_ascii(char *s) {
  while (*s) {
    *s = (char)tolower((unsigned char)*s);
    s++;
  }
}

#if defined(ULISP_WIFI) && defined(ULISP_HTTP_ARDUINO_TRANSPORT)
void close_active_client() {
  if (HttpClientActive) HttpClient.stop(100);
  HttpClient = WiFiClient();
  HttpClientActive = false;
  HttpAwaitingResponse = false;
  HttpRequestIsHead = false;
  HttpRequestLength = 0;
  HttpRequestBuffer[0] = '\0';
}

void write_bytes(const char *s, size_t len) {
  if (len == 0) return;
  HttpClient.write((const uint8_t *)s, len);
}

void write_cstr(const char *s) {
  write_bytes(s, strlen(s));
}

void write_int(int n) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%d", n);
  write_cstr(buffer);
}

const char *reason_phrase(int status) {
  switch (status) {
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 304: return "Not Modified";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 431: return "Request Header Fields Too Large";
    case 500: return "Internal Server Error";
    case 503: return "Service Unavailable";
    default: return "OK";
  }
}

void write_error_and_close(int status, const char *message) {
  if (!HttpClientActive) return;
  size_t len = strlen(message);
  write_cstr("HTTP/1.1 ");
  write_int(status);
  write_cstr(" ");
  write_cstr(reason_phrase(status));
  write_cstr("\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: ");
  write_int((int)len);
  write_cstr("\r\nConnection: close\r\n\r\n");
  write_bytes(message, len);
  close_active_client();
}

int find_header_end() {
  for (size_t i = 0; i < HttpRequestLength; i++) {
    if (i + 3 < HttpRequestLength && HttpRequestBuffer[i] == '\r' && HttpRequestBuffer[i + 1] == '\n' &&
        HttpRequestBuffer[i + 2] == '\r' && HttpRequestBuffer[i + 3] == '\n') {
      return (int)(i + 4);
    }
    if (i + 1 < HttpRequestLength && HttpRequestBuffer[i] == '\n' && HttpRequestBuffer[i + 1] == '\n') {
      return (int)(i + 2);
    }
  }
  return -1;
}

char *next_line(char *&cursor) {
  if (cursor == nullptr || *cursor == '\0') return nullptr;
  char *line = cursor;
  char *nl = strchr(cursor, '\n');
  if (nl != nullptr) {
    *nl = '\0';
    cursor = nl + 1;
  } else {
    cursor = nullptr;
  }
  size_t len = strlen(line);
  if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';
  return line;
}

object *build_headers_alist(char *cursor) {
  object *headers = nil;
  while (true) {
    char *line = next_line(cursor);
    if (line == nullptr) break;
    if (*line == '\0') break;
    if (strlen(line) > kMaxHeaderLine) error2("http header too long");
    char *colon = strchr(line, ':');
    if (colon == nullptr) continue;
    *colon = '\0';
    char *name = line;
    char *value = colon + 1;
    trim_in_place(name);
    trim_in_place(value);
    lower_ascii(name);
    headers = cons(cons(make_string(name), make_string(value)), headers);
  }
  return headers;
}

object *parse_request_to_lisp(int header_end) {
  HttpRequestBuffer[header_end] = '\0';
  char *cursor = HttpRequestBuffer;
  char *request_line = next_line(cursor);
  if (request_line == nullptr || *request_line == '\0') return nil;

  char *method = request_line;
  char *target = strchr(method, ' ');
  if (target == nullptr) return nil;
  *target++ = '\0';
  while (*target == ' ') target++;
  char *version = strchr(target, ' ');
  if (version == nullptr) return nil;
  *version++ = '\0';
  while (*version == ' ') version++;
  if (*method == '\0' || *target == '\0' || *version == '\0') return nil;

  HttpRequestIsHead = (strcasecmp(method, "HEAD") == 0);

  char *query = strchr(target, '?');
  if (query != nullptr) *query++ = '\0';
  else query = (char *)"";

  object *headers = build_headers_alist(cursor);
  object *request = nil;
  request = alist_put(request, "client", number(1));
  request = alist_put(request, "body", make_string(""));
  request = alist_put(request, "headers", headers);
  request = alist_put(request, "version", make_string(version));
  request = alist_put(request, "query", make_string(query));
  request = alist_put(request, "path", make_string(target));
  request = alist_put(request, "method", make_string(method));
  return request;
}

void write_lisp_string(object *str) {
  str = cdr(checkstring(str));
  char chunk[kWriteChunkSize];
  size_t n = 0;
  while (str != NULL) {
    uint32_t chars = str->chars;
    for (int shift = (sizeof(int) - 1) * 8; shift >= 0; shift -= 8) {
      char ch = (char)((chars >> shift) & 0xFF);
      if (ch == 0) continue;
      chunk[n++] = ch;
      if (n == sizeof(chunk)) {
        write_bytes(chunk, n);
        n = 0;
      }
    }
    str = car(str);
  }
  if (n) write_bytes(chunk, n);
}

void write_extra_headers(object *headers) {
  while (headers != NULL) {
    object *entry = first(headers);
    if (!consp(entry) || !stringp(car(entry)) || !stringp(cdr(entry))) error2("invalid headers");
    write_lisp_string(car(entry));
    write_cstr(": ");
    write_lisp_string(cdr(entry));
    write_cstr("\r\n");
    headers = cdr(headers);
  }
}
#endif

int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

object *transform_string(object *arg, bool html_escape) {
  arg = checkstring(arg);
  object *out = startstring();
  int len = stringlength(arg);
  for (int i = 0; i < len; i++) {
    char ch = (char)nthchar(arg, i);
    if (html_escape) {
      switch (ch) {
        case '&': pfstring("&amp;", pstr); break;
        case '<': pfstring("&lt;", pstr); break;
        case '>': pfstring("&gt;", pstr); break;
        case '"': pfstring("&quot;", pstr); break;
        case '\'': pfstring("&#39;", pstr); break;
        default: pstr(ch); break;
      }
    } else {
      if (ch == '+') pstr(' ');
      else if (ch == '%' && i + 2 < len) {
        int hi = hex_value((char)nthchar(arg, i + 1));
        int lo = hex_value((char)nthchar(arg, i + 2));
        if (hi >= 0 && lo >= 0) {
          pstr((char)((hi << 4) | lo));
          i += 2;
        } else {
          pstr(ch);
        }
      } else {
        pstr(ch);
      }
    }
  }
  return out;
}

} // namespace

object *fn_httpserverstart(object *args, object *env) {
  (void)env;
#if defined(ULISP_WIFI) && defined(ULISP_HTTP_ARDUINO_TRANSPORT)
  if (args != NULL) {
    int port = checkinteger(first(args));
    if (port != kHttpPort) error2("only port 80 supported");
  }
  if (!HttpServerEnabled) {
    HttpServer.begin(kHttpPort);
    HttpServerEnabled = true;
  }
  return number(kHttpPort);
#else
  (void)args;
  error2("not supported");
  return nil;
#endif
}

object *fn_httpserverstop(object *args, object *env) {
  (void)args;
  (void)env;
#if defined(ULISP_WIFI) && defined(ULISP_HTTP_ARDUINO_TRANSPORT)
  close_active_client();
  HttpServer.stop();
  HttpServerEnabled = false;
#endif
  return nil;
}

object *fn_httpaccept(object *args, object *env) {
  (void)args;
  (void)env;
#if defined(ULISP_WIFI) && defined(ULISP_HTTP_ARDUINO_TRANSPORT)
  if (!HttpServerEnabled || HttpAwaitingResponse) return nil;

  if (!HttpClientActive) {
    HttpClient = HttpServer.accept();
    if (!HttpClient) return nil;
    HttpClient.setSync(true);
    HttpClient.setNoDelay(true);
    HttpClientActive = true;
    HttpRequestLength = 0;
    HttpRequestBuffer[0] = '\0';
    HttpClientAcceptedAt = millis();
  }

  if (millis() - HttpClientAcceptedAt > kClientTimeoutMs) {
    close_active_client();
    return nil;
  }

  unsigned long deadline = millis() + kParseBudgetMs;
  while (HttpClientActive && millis() <= deadline && HttpClient.available() > 0) {
    if (HttpRequestLength >= kRequestBufferSize) {
      write_error_and_close(431, "request headers too large");
      return nil;
    }
    int ch = HttpClient.read();
    if (ch < 0) break;
    HttpRequestBuffer[HttpRequestLength++] = (char)ch;
    HttpRequestBuffer[HttpRequestLength] = '\0';
    int header_end = find_header_end();
    if (header_end >= 0) {
      object *request = parse_request_to_lisp(header_end);
      if (request == nil) {
        write_error_and_close(400, "bad request");
        return nil;
      }
      HttpAwaitingResponse = true;
      return request;
    }
  }

  if (HttpClientActive && !HttpClient.connected() && HttpClient.available() <= 0) close_active_client();
  return nil;
#else
  return nil;
#endif
}

object *fn_httprespond(object *args, object *env) {
  (void)env;
#if defined(ULISP_WIFI) && defined(ULISP_HTTP_ARDUINO_TRANSPORT)
  if (!HttpClientActive || !HttpAwaitingResponse) error2("no active http request");

  int status = checkinteger(first(args));
  char content_type[96];
  cstring(second(args), content_type, sizeof(content_type));
  object *body = checkstring(third(args));
  object *headers = nil;
  object *more = cdr(cddr(args));
  if (more != NULL) headers = first(more);
  if (headers != nil && !listp(headers)) error2("invalid headers");

  int body_len = stringlength(body);
  write_cstr("HTTP/1.1 ");
  write_int(status);
  write_cstr(" ");
  write_cstr(reason_phrase(status));
  write_cstr("\r\nContent-Type: ");
  write_cstr(content_type);
  write_cstr("\r\nContent-Length: ");
  write_int(body_len);
  write_cstr("\r\nConnection: close\r\n");
  write_extra_headers(headers);
  write_cstr("\r\n");
  if (!HttpRequestIsHead) write_lisp_string(body);
  close_active_client();
  return nil;
#else
  (void)args;
  error2("not supported");
  return nil;
#endif
}

object *fn_urldecode(object *args, object *env) {
  (void)env;
  return transform_string(first(args), false);
}

object *fn_htmlescape(object *args, object *env) {
  (void)env;
  return transform_string(first(args), true);
}
