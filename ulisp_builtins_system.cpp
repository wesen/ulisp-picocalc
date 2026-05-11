
#include <Arduino.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <Wire.h>

#include "ulisp_config.h"

#if defined(ULISP_WIFI)
#include "pico/cyw43_arch.h"
#include "lwip/dhcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
extern "C" {
#include "dhcpserver.h"
#include "dnsserver.h"
}
#endif

#if defined(sdcardsupport)
#include <SD.h>
#endif

#define Serial Serial1

#include "ulisp_types.h"
#include "ulisp_messages.h"
#include "ulisp_state.h"
#include "ulisp_error.h"
#include "ulisp_memory.h"
#include "ulisp_persistence.h"
#include "ulisp_runtime.h"
#include "ulisp_streams.h"
#include "ulisp_platform.h"
#include "ulisp_pretty.h"
#include "ulisp_print.h"
#include "ulisp_tables.h"
#include "ulisp_eval.h"
#include "ulisp_reader.h"
#include "ulisp_builtins.h"
#include "ulisp_entry.h"

extern const char notanumber[];
extern const char notaninteger[];
extern const char notastring[];
extern const char notalist[];
extern const char notasymbol[];
extern const char notproper[];
extern const char toomanyargs[];
extern const char toofewargs[];
extern const char noargument[];
extern const char nostream[];
extern const char overflow[];
extern const char divisionbyzero[];
extern const char indexnegative[];
extern const char invalidarg[];
extern const char invalidkey[];
extern const char illegalclause[];
extern const char illegalfn[];
extern const char oddargs[];
extern const char indexrange[];
extern const char unknownstreamtype[];

#if defined(ULISP_WIFI)
namespace {
constexpr uint32_t kWifiConnectTimeoutMs = 60000;
extern bool WifiApRunning;
extern dhcp_server_t WifiDhcpServer;
extern dns_server_t WifiDnsServer;
extern ip4_addr_t WifiApGw;
static void wifi_debugf(const char *fmt, ...);
static void wifi_set_trace(bool enabled);
static void wifi_log_state(const char *prefix);
static void ip4_to_cstr(const ip4_addr_t *addr, char *buffer, size_t len);
static object *ip4_to_lisp(const ip4_addr_t *addr);
static void wifi_stop_ap();
}
#endif

// System functions

object *fn_eval (object *args, object *env) {
  return eval(first(args), env);
}

object *fn_return (object *args, object *env) {
  (void) env;
  setflag(RETURNFLAG);
  if (args == NULL) return nil; else return first(args);
}

object *fn_globals (object *args, object *env) {
  (void) args, (void) env;
  object *result = cons(NULL, NULL);
  object *ptr = result;
  object *arg = GlobalEnv;
  while (arg != NULL) {
    cdr(ptr) = cons(car(car(arg)), NULL); ptr = cdr(ptr);
    arg = cdr(arg);
  }
  return cdr(result);
}

object *fn_locals (object *args, object *env) {
  (void) args;
  return env;
}

object *fn_makunbound (object *args, object *env) {
  (void) env;
  object *var = first(args);
  if (!symbolp(var)) error(notasymbol, var);
  delassoc(var, &GlobalEnv);
  return var;
}

object *fn_break (object *args, object *env) {
  (void) args;
  pfstring("\nBreak!\n", pserial);
  BreakLevel++;
  repl(env);
  BreakLevel--;
  return nil;
}

object *fn_read (object *args, object *env) {
  (void) env;
  gfun_t gfun = gstreamfun(args);
  return readmain(gfun);
}

object *fn_prin1 (object *args, object *env) {
  (void) env;
  object *obj = first(args);
  pfun_t pfun = pstreamfun(cdr(args));
  printobject(obj, pfun);
  return obj;
}

object *fn_print (object *args, object *env) {
  (void) env;
  object *obj = first(args);
  pfun_t pfun = pstreamfun(cdr(args));
  pln(pfun);
  printobject(obj, pfun);
  pfun(' ');
  return obj;
}

object *fn_princ (object *args, object *env) {
  (void) env;
  object *obj = first(args);
  pfun_t pfun = pstreamfun(cdr(args));
  prin1object(obj, pfun);
  return obj;
}

object *fn_terpri (object *args, object *env) {
  (void) env;
  pfun_t pfun = pstreamfun(args);
  pln(pfun);
  return nil;
}

object *fn_readbyte (object *args, object *env) {
  (void) env;
  gfun_t gfun = gstreamfun(args);
  if (gfun == gserial) gserial_flush();
  int c = gfun();
  return (c == -1) ? nil : number(c);
}

object *fn_readline (object *args, object *env) {
  (void) env;
  gfun_t gfun = gstreamfun(args);
  if (gfun == gserial) gserial_flush();
  return readstring('\n', false, gfun);
}

object *fn_writebyte (object *args, object *env) {
  (void) env;
  int c = checkinteger(first(args));
  pfun_t pfun = pstreamfun(cdr(args));
  if (c == '\n' && pfun == pserial) Serial.write('\n');
  else (pfun)(c);
  return nil;
}

object *fn_writestring (object *args, object *env) {
  (void) env;
  object *obj = first(args);
  pfun_t pfun = pstreamfun(cdr(args));
  flags_t temp = Flags;
  clrflag(PRINTREADABLY);
  printstring(obj, pfun);
  Flags = temp;
  return nil;
}

object *fn_writeline (object *args, object *env) {
  (void) env;
  object *obj = first(args);
  pfun_t pfun = pstreamfun(cdr(args));
  flags_t temp = Flags;
  clrflag(PRINTREADABLY);
  printstring(obj, pfun);
  pln(pfun);
  Flags = temp;
  return nil;
}

object *fn_restarti2c (object *args, object *env) {
  (void) env;
  int stream = isstream(first(args));
  args = cdr(args);
  int read = 0; // Write
  I2Ccount = 0;
  if (args != NULL) {
    object *rw = first(args);
    if (integerp(rw)) I2Ccount = rw->integer;
    read = (rw != NULL);
  }
  int address = stream & 0xFF;
  if (stream>>8 != I2CSTREAM) error2("not an i2c stream");
  TwoWire *port;
  if (address < 128) port = &Wire;
  #if ULISP_HOWMANYI2C == 2
  else port = &Wire1;
  #endif
  return I2Crestart(port, address & 0x7F, read) ? tee : nil;
}

object *fn_gc (object *args, object *env) {
  if (args == NULL || first(args) != NULL) {
    int initial = Freespace;
    unsigned long start = micros();
    gc(args, env);
    unsigned long elapsed = micros() - start;
    pfstring("Space: ", pserial);
    pint(Freespace - initial, pserial);
    pfstring(" bytes, Time: ", pserial);
    pint(elapsed, pserial);
    pfstring(" us\n", pserial);
  } else gc(args, env);
  return nil;
}

object *fn_room (object *args, object *env) {
  (void) args, (void) env;
  return number(Freespace);
}

object *fn_backtrace (object *args, object *env) {
  (void) env;
  if (args == NULL) return (tstflag(BACKTRACE)) ? tee : nil;
  if (first(args) == NULL) clrflag(BACKTRACE); else setflag(BACKTRACE);
  return first(args);
}

object *fn_saveimage (object *args, object *env) {
  if (args != NULL) args = eval(first(args), env);
  return number(saveimage(args));
}

object *fn_loadimage (object *args, object *env) {
  (void) env;
  if (args != NULL) args = first(args);
  return number(loadimage(args));
}

object *fn_cls (object *args, object *env) {
  (void) args, (void) env;
  pserial(12);
  return nil;
}



// Tree Editor

object *fn_edit (object *args, object *env) {
  object *fun = first(args);
  object *pair = findvalue(fun, env);
  clrflag(EXITEDITOR);
  object *arg = edit(eval(fun, env));
  cdr(pair) = arg;
  return arg;
}

// Pretty printer

object *fn_pprint (object *args, object *env) {
  (void) env;
  object *obj = first(args);
  pfun_t pfun = pstreamfun(cdr(args));
  #if defined(gfxsupport)
  if (pfun == gfxwrite) ppwidth = GFXPPWIDTH;
  #endif
  pln(pfun);
  superprint(obj, 0, pfun);
  ppwidth = PPWIDTH;
  return bsymbol(NOTHING);
}

object *fn_pprintall (object *args, object *env) {
  (void) env;
  pfun_t pfun = pstreamfun(args);
  #if defined(gfxsupport)
  if (pfun == gfxwrite) ppwidth = GFXPPWIDTH;
  #endif
  object *globals = GlobalEnv;
  while (globals != NULL) {
    object *pair = first(globals);
    object *var = car(pair);
    object *val = cdr(pair);
    pln(pfun);
    if (consp(val) && symbolp(car(val)) && builtin(car(val)->name) == LAMBDA) {
      superprint(cons(bsymbol(DEFUN), cons(var, cdr(val))), 0, pfun);
    } else if (consp(val) && car(val)->type == CODE) {
      superprint(cons(bsymbol(DEFCODE), cons(var, cdr(val))), 0, pfun);
    } else {
      superprint(cons(bsymbol(DEFVAR), cons(var, cons(quote(val), NULL))), 0, pfun);
    }
    pln(pfun);
    testescape();
    globals = cdr(globals);
  }
  ppwidth = PPWIDTH;
  return bsymbol(NOTHING);
}

// Format

object *fn_format (object *args, object *env) {
  (void) env;
  pfun_t pfun = pserial;
  object *output = first(args);
  object *obj;
  if (output == nil) { obj = startstring(); pfun = pstr; }
  else if (!eq(output, tee)) pfun = pstreamfun(args);
  object *formatstr = checkstring(second(args));
  object *save = NULL;
  args = cddr(args);
  uint16_t len = stringlength(formatstr);
  uint16_t n = 0, width = 0, w, bra = 0;
  char pad = ' ';
  bool tilde = false, mute = false, comma = false, quote = false;
  while (n < len) {
    char ch = nthchar(formatstr, n);
    char ch2 = ch & ~0x20; // force to upper case
    if (tilde) {
     if (ch == '}') {
        if (save == NULL) formaterr(formatstr, "no matching ~{", n);
        if (args == NULL) { args = cdr(save); save = NULL; } else n = bra;
        mute = false; tilde = false;
      }
      else if (!mute) {
        if (comma && quote) { pad = ch; comma = false, quote = false; }
        else if (ch == '\'') {
          if (comma) quote = true;
          else formaterr(formatstr, "quote not valid", n);
        }
        else if (ch == '~') { pfun('~'); tilde = false; }
        else if (ch >= '0' && ch <= '9') width = width*10 + ch - '0';
        else if (ch == ',') comma = true;
        else if (ch == '%') { pln(pfun); tilde = false; }
        else if (ch == '&') { pfl(pfun); tilde = false; }
        else if (ch == '^') {
          if (save != NULL && args == NULL) mute = true;
          tilde = false;
        }
        else if (ch == '{') {
          if (save != NULL) formaterr(formatstr, "can't nest ~{", n);
          if (args == NULL) formaterr(formatstr, noargument, n);
          if (!listp(first(args))) formaterr(formatstr, notalist, n);
          save = args; args = first(args); bra = n; tilde = false;
          if (args == NULL) mute = true;
        }
        else if (ch2 == 'A' || ch2 == 'S' || ch2 == 'D' || ch2 == 'G' || ch2 == 'X' || ch2 == 'B') {
          if (args == NULL) formaterr(formatstr, noargument, n);
          object *arg = first(args); args = cdr(args);
          uint8_t aw = atomwidth(arg);
          if (width < aw) w = 0; else w = width-aw;
          tilde = false;
          if (ch2 == 'A') { prin1object(arg, pfun); indent(w, pad, pfun); }
          else if (ch2 == 'S') { printobject(arg, pfun); indent(w, pad, pfun); }
          else if (ch2 == 'D' || ch2 == 'G') { indent(w, pad, pfun); prin1object(arg, pfun); }
          else if (ch2 == 'X' || ch2 == 'B') {
            if (integerp(arg)) {
              uint8_t base = (ch2 == 'B') ? 2 : 16;
              uint8_t hw = basewidth(arg, base); if (width < hw) w = 0; else w = width-hw;
              indent(w, pad, pfun); pintbase(arg->integer, base, pfun);
            } else {
              indent(w, pad, pfun); prin1object(arg, pfun);
            }
          }
          tilde = false;
        } else formaterr(formatstr, "invalid directive", n);
      }
    } else {
      if (ch == '~') { tilde = true; pad = ' '; width = 0; comma = false; quote = false; }
      else if (!mute) pfun(ch);
    }
    n++;
  }
  if (output == nil) return obj;
  else return nil;
}

// LispLibrary

object *fn_require (object *args, object *env) {
  object *arg = first(args);
  object *globals = GlobalEnv;
  if (!symbolp(arg)) error(notasymbol, arg);
  while (globals != NULL) {
    object *pair = first(globals);
    object *var = car(pair);
    if (symbolp(var) && var == arg) return nil;
    globals = cdr(globals);
  }
  GlobalStringIndex = 0;
  object *line = readmain(glibrary);
  while (line != NULL) {
    // Is this the definition we want
    symbol_t fname = first(line)->name;
    if ((fname == sym(DEFUN) || fname == sym(DEFVAR)) && symbolp(second(line)) && second(line)->name == arg->name) {
      eval(line, env);
      return tee;
    }
    line = readmain(glibrary);
  }
  return nil;
}

object *fn_listlibrary (object *args, object *env) {
  (void) args, (void) env;
  GlobalStringIndex = 0;
  object *line = readmain(glibrary);
  while (line != NULL) {
    builtin_t bname = builtin(first(line)->name);
    if (bname == DEFUN || bname == DEFVAR) {
      printsymbol(second(line), pserial); pserial(' ');
    }
    line = readmain(glibrary);
  }
  return bsymbol(NOTHING);
}

// Documentation

object *sp_help (object *args, object *env) {
  if (args == NULL) error2(noargument);
  object *docstring = documentation(first(args), env);
  if (docstring) {
    flags_t temp = Flags;
    clrflag(PRINTREADABLY);
    printstring(docstring, pserial);
    Flags = temp;
  }
  return bsymbol(NOTHING);
}

object *fn_documentation (object *args, object *env) {
  return documentation(first(args), env);
}

object *fn_apropos (object *args, object *env) {
  (void) env;
  apropos(first(args), true);
  return bsymbol(NOTHING);
}

object *fn_aproposlist (object *args, object *env) {
  (void) env;
  return apropos(first(args), false);
}

// Error handling

object *sp_unwindprotect (object *args, object *env) {
  if (args == NULL) error2(toofewargs);
  object *current_GCStack = GCStack;
  jmp_buf dynamic_handler;
  jmp_buf *previous_handler = handler;
  handler = &dynamic_handler;
  object *protected_form = first(args);
  object *result;

  bool signaled = false;
  if (!setjmp(dynamic_handler)) {
    result = eval(protected_form, env);
  } else {
    GCStack = current_GCStack;
    signaled = true;
  }
  handler = previous_handler;

  object *protective_forms = cdr(args);
  while (protective_forms != NULL) {
    eval(car(protective_forms), env);
    if (tstflag(RETURNFLAG)) break;
    protective_forms = cdr(protective_forms);
  }

  if (!signaled) return result;
  GCStack = NULL;
  longjmp(*handler, 1);
}

object *sp_ignoreerrors (object *args, object *env) {
  object *current_GCStack = GCStack;
  jmp_buf dynamic_handler;
  jmp_buf *previous_handler = handler;
  handler = &dynamic_handler;
  object *result = nil;

  bool muffled = tstflag(MUFFLEERRORS);
  setflag(MUFFLEERRORS);
  bool signaled = false;
  if (!setjmp(dynamic_handler)) {
    while (args != NULL) {
      result = eval(car(args), env);
      if (tstflag(RETURNFLAG)) break;
      args = cdr(args);
    }
  } else {
    GCStack = current_GCStack;
    signaled = true;
  }
  handler = previous_handler;
  if (!muffled) clrflag(MUFFLEERRORS);

  if (signaled) return bsymbol(NOTHING);
  else return result;
}

object *sp_error (object *args, object *env) {
  object *message = eval(cons(bsymbol(FORMAT), cons(nil, args)), env);
  if (!tstflag(MUFFLEERRORS)) {
    flags_t temp = Flags;
    clrflag(PRINTREADABLY);
    pfstring("Error: ", pserial); printstring(message, pserial);
    Flags = temp;
    pln(pserial);
  }
  GCStack = NULL;
  longjmp(*handler, 1);
}

// SD Card utilities

object *fn_directory (object *args, object *env) {
  (void) args, (void) env;
  #if defined(sdcardsupport)
  SDBegin();
  File root = SD.open("/");
  if (!root) error2("problem reading from SD card");
  object *result = cons(NULL, NULL);
  object *ptr = result;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    object *filename = lispstring((char*)entry.name());
    cdr(ptr) = cons(filename, NULL);
    ptr = cdr(ptr);
    entry.close();
  }
  root.close();
  return cdr(result);
  #else
  error2("not supported");
  return nil;
  #endif
}

// Wi-Fi

object *sp_withclient (object *args, object *env) {
  (void) args, (void) env;
  error2("wifi streams disabled");
  return nil;
}

object *fn_available (object *args, object *env) {
  (void) args, (void) env;
  error2("wifi streams disabled");
  return nil;
}

object *fn_wifiserver (object *args, object *env) {
  (void) args, (void) env;
  error2("wifi streams disabled");
  return nil;
}

object *fn_wifisoftap (object *args, object *env) {
  #if defined (ULISP_WIFI)
  (void) env;
  char ssid[33], pass[65];
  object *ssid_arg = first(args);
  object *pass_arg = second(args);
  wifi_stop_ap();
  const char *pass_cstr = nullptr;
  uint32_t auth = CYW43_AUTH_OPEN;
  if (pass_arg != NULL && pass_arg != nil) {
    pass_cstr = cstring(pass_arg, pass, sizeof(pass));
    if (strlen(pass_cstr) >= 8) auth = CYW43_AUTH_WPA2_AES_PSK;
    else pass_cstr = nullptr;
  }
  wifi_debugf("softap ssid='%s' auth=%s\n", cstring(ssid_arg, ssid, sizeof(ssid)), auth == CYW43_AUTH_OPEN ? "open" : "wpa2-aes");
  cyw43_arch_enable_ap_mode(ssid, pass_cstr, auth);
#if LWIP_IPV6
#define ULISP_IP4(x) ((x).u_addr.ip4)
#else
#define ULISP_IP4(x) (x)
#endif
  ip4_addr_t mask;
  ULISP_IP4(WifiApGw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
  ULISP_IP4(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
#undef ULISP_IP4
  dhcp_server_init(&WifiDhcpServer, &WifiApGw, &mask);
  dns_server_init(&WifiDnsServer, &WifiApGw);
  WifiApRunning = true;
  char ap_ip[16];
  ip4_to_cstr(&WifiApGw, ap_ip, sizeof(ap_ip));
  wifi_debugf("softap ip=%s dhcp=on dns=on\n", ap_ip);
  return ip4_to_lisp(&WifiApGw);
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_connected (object *args, object *env) {
  (void) args, (void) env;
  error2("wifi streams disabled");
  return nil;
}

object *fn_wifilocalip (object *args, object *env) {
  #if defined (ULISP_WIFI)
  (void) args, (void) env;
  return ip4_to_lisp(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA]));
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wificonnect (object *args, object *env) {
  #if defined (ULISP_WIFI)
  (void) env;
  char ssid[33], pass[65];
  if (args == NULL) { wifi_debugf("disconnect requested\n"); cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA); return nil; }
  cyw43_arch_enable_sta_mode();
  wifi_set_trace(true);
  wifi_log_state("connect-before");
  uint32_t auth = CYW43_AUTH_WPA2_AES_PSK;
  const char *pass_cstr = "";
  if (cdr(args) == NULL) auth = CYW43_AUTH_OPEN;
  else pass_cstr = cstring(second(args), pass, sizeof(pass));
  wifi_debugf("connecting ssid='%s' auth=%s timeout=%lums\n", cstring(first(args), ssid, sizeof(ssid)), auth == CYW43_AUTH_OPEN ? "open" : "wpa2-aes", (unsigned long)kWifiConnectTimeoutMs);
  int result = cyw43_arch_wifi_connect_timeout_ms(ssid, pass_cstr, auth, kWifiConnectTimeoutMs);
  wifi_debugf("connect result=%d\n", result);
  wifi_log_state("connect-after");
  wifi_set_trace(false);
  if (result == PICO_OK) return ip4_to_lisp(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA]));
  if (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_JOIN) error2("connected but no ip");
  else if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_BADAUTH) error2("bad authentication");
  else if (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_NONET) error2("network not found");
  else error2("unable to connect");
  return nil;
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

#if defined(ULISP_WIFI)
namespace {
constexpr size_t kWifiMaxScanResults = 32;
constexpr uint32_t kWifiScanTimeoutMs = 30000;

struct WifiScanResult {
  char ssid[33];
  int16_t rssi;
  uint8_t channel;
  uint8_t security;
  uint8_t bssid[6];
};

WifiScanResult WifiScanResults[kWifiMaxScanResults];
static object *wifi_scan_entry(const WifiScanResult &r);
volatile size_t WifiScanCount = 0;
bool WifiScanEverStarted = false;
int WifiLastScanStartResult = 0;
bool WifiApRunning = false;
bool WifiTraceEnabled = false;
dhcp_server_t WifiDhcpServer;
dns_server_t WifiDnsServer;
ip4_addr_t WifiApGw;

static uint32_t wifi_now_ms() {
  return to_ms_since_boot(get_absolute_time());
}

static void wifi_debugf(const char *fmt, ...) {
  char message[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(message, sizeof(message), fmt, args);
  va_end(args);
  Serial.printf("[%10lu ms] wifi: ", (unsigned long)wifi_now_ms());
  for (char *p = message; *p; p++) {
    if (*p == '\n') Serial.write('\r');
    Serial.write(*p);
  }
}

static void wifi_set_trace(bool enabled) {
  WifiTraceEnabled = enabled;
  if (enabled) cyw43_state.trace_flags |= CYW43_TRACE_ASYNC_EV;
  else cyw43_state.trace_flags &= ~CYW43_TRACE_ASYNC_EV;
  wifi_debugf("async trace %s\n", enabled ? "on" : "off");
}

static void wifi_clear_reader_pushback(const char *where) {
  if (LastChar) {
    wifi_debugf("clearing reader pushback at %s char=0x%02x\n", where, (unsigned char)LastChar);
    LastChar = 0;
  }
}

static void ip4_to_cstr(const ip4_addr_t *addr, char *buffer, size_t len) {
  ip4addr_ntoa_r(addr, buffer, (int)len);
}

static object *ip4_to_lisp(const ip4_addr_t *addr) {
  char buffer[16];
  ip4_to_cstr(addr, buffer, sizeof(buffer));
  return lispstring(buffer);
}

static const char *wifi_link_status_name(int status) {
  switch (status) {
    case CYW43_LINK_DOWN: return "down";
    case CYW43_LINK_JOIN: return "joined";
    case CYW43_LINK_NOIP: return "no-ip";
    case CYW43_LINK_UP: return "up";
    case CYW43_LINK_FAIL: return "fail";
    case CYW43_LINK_NONET: return "no-net";
    case CYW43_LINK_BADAUTH: return "bad-auth";
    default: return "unknown";
  }
}

static const char *wifi_scan_security_name(uint8_t security) {
  switch (security) {
    case 0: return "open";
    case 1: return "wep/privacy";
    case 2: return "wpa";
    case 3: return "wpa+wep/privacy";
    case 4: return "wpa2";
    case 5: return "wpa2+privacy";
    case 6: return "wpa+wpa2";
    case 7: return "wpa+wpa2+privacy";
    default: return "unknown";
  }
}

static const char *wifi_dhcp_state_name(int state) {
  switch (state) {
    case -1: return "none";
    case 0: return "off";
    case 1: return "requesting";
    case 2: return "init";
    case 3: return "rebooting";
    case 4: return "rebinding";
    case 5: return "renewing";
    case 6: return "selecting";
    case 7: return "informing";
    case 8: return "checking";
    case 9: return "bound";
    case 10: return "backing-off";
    default: return "unknown";
  }
}

static object *alist_string(const char *key, object *value, object *tail) {
  return cons(cons(lispstring((char *)key), value), tail);
}

static int wifi_scan_callback(void *, const cyw43_ev_scan_result_t *result) {
  if (!result) return 0;
  wifi_debugf("scan result ssid='%s' rssi=%d chan=%u sec=%u(%s) bssid=%02x:%02x:%02x:%02x:%02x:%02x\n",
              result->ssid, result->rssi, result->channel, result->auth_mode,
              wifi_scan_security_name(result->auth_mode),
              result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5]);
  size_t index = WifiScanCount;
  if (index >= kWifiMaxScanResults) return 0;
  WifiScanCount = index + 1;
  WifiScanResult &out = WifiScanResults[index];
  strncpy(out.ssid, (const char *)result->ssid, sizeof(out.ssid) - 1);
  out.ssid[sizeof(out.ssid) - 1] = '\0';
  out.rssi = result->rssi;
  out.channel = result->channel;
  out.security = result->auth_mode;
  memcpy(out.bssid, result->bssid, sizeof(out.bssid));
  return 0;
}

static int wifi_start_scan(bool clear_results) {
  wifi_clear_reader_pushback("scan-start-entry");
  wifi_debugf("scan-start requested clear=%d active=%d count=%u\n", clear_results ? 1 : 0, cyw43_wifi_scan_active(&cyw43_state) ? 1 : 0, (unsigned)WifiScanCount);
  cyw43_arch_enable_sta_mode();
  if (clear_results) WifiScanCount = 0;
  cyw43_wifi_scan_options_t options{};
  if (cyw43_wifi_scan_active(&cyw43_state)) {
    wifi_debugf("scan already active; kicking CYW43 scan path to flush callbacks\n");
  }
  WifiLastScanStartResult = cyw43_wifi_scan(&cyw43_state, &options, nullptr, wifi_scan_callback);
  WifiScanEverStarted = (WifiLastScanStartResult == 0);
  wifi_debugf("scan-start result=%d active=%d\n", WifiLastScanStartResult, cyw43_wifi_scan_active(&cyw43_state) ? 1 : 0);
  wifi_clear_reader_pushback("scan-start-exit");
  return WifiLastScanStartResult;
}

static object *wifi_results_list() {
  size_t n = WifiScanCount;
  object *result = nil;
  for (int i = (int)n - 1; i >= 0; i--) result = cons(wifi_scan_entry(WifiScanResults[i]), result);
  return result;
}

static void wifi_stop_ap() {
  if (!WifiApRunning) return;
  wifi_debugf("stopping softap\n");
  dns_server_deinit(&WifiDnsServer);
  dhcp_server_deinit(&WifiDhcpServer);
  cyw43_arch_disable_ap_mode();
  WifiApRunning = false;
}

static void wifi_log_state(const char *prefix) {
  netif *sta = &cyw43_state.netif[CYW43_ITF_STA];
  int wifi_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
  int tcpip_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
#if LWIP_DHCP
  dhcp *dhcp_state = netif_dhcp_data(sta);
  int dhcp_state_num = dhcp_state ? dhcp_state->state : -1;
  int dhcp_tries = dhcp_state ? dhcp_state->tries : -1;
#else
  int dhcp_state_num = -1;
  int dhcp_tries = -1;
#endif
  int32_t rssi = 0;
  int rssi_err = cyw43_wifi_get_rssi(&cyw43_state, &rssi);
  char ip[16], gw[16], mask[16];
  ip4_to_cstr(netif_ip4_addr(sta), ip, sizeof(ip));
  ip4_to_cstr(netif_ip4_gw(sta), gw, sizeof(gw));
  ip4_to_cstr(netif_ip4_netmask(sta), mask, sizeof(mask));
  wifi_debugf("%s wifi=%d(%s) tcpip=%d(%s) netif-up=%d link-up=%d dhcp=%d(%s) tries=%d ip=%s gw=%s mask=%s rssi=%ld%s\n",
              prefix,
              wifi_status, wifi_link_status_name(wifi_status),
              tcpip_status, wifi_link_status_name(tcpip_status),
              netif_is_up(sta) ? 1 : 0,
              netif_is_link_up(sta) ? 1 : 0,
              dhcp_state_num, wifi_dhcp_state_name(dhcp_state_num), dhcp_tries,
              ip, gw, mask, (long)rssi, rssi_err ? "(unavailable)" : "");
}

static object *wifi_scan_entry(const WifiScanResult &r) {
  object *ssid = lispstring((char *)r.ssid);
  object *entry = cons(ssid, cons(number(r.rssi), cons(number(r.security), cons(number(r.channel), nil))));
  return entry;
}
} // namespace
#endif

object *fn_wifiscan (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) args, (void) env;
  if (!WifiScanEverStarted && WifiScanCount == 0) wifi_start_scan(true);
  wifi_clear_reader_pushback("wifi-scan-return");
  return wifi_results_list();
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifiscanstart (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) env;
  bool clear_results = (args == NULL || first(args) != nil);
  return number(wifi_start_scan(clear_results));
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifiscanactive (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) args, (void) env;
  cyw43_arch_poll();
  bool active = cyw43_wifi_scan_active(&cyw43_state);
  wifi_debugf("scan-active active=%d count=%u last-start=%d\n", active ? 1 : 0, (unsigned)WifiScanCount, WifiLastScanStartResult);
  return active ? tee : nil;
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifiscanresults (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) args, (void) env;
  wifi_debugf("scan-results count=%u active=%d\n", (unsigned)WifiScanCount, cyw43_wifi_scan_active(&cyw43_state) ? 1 : 0);
  wifi_clear_reader_pushback("wifi-scan-results-return");
  return wifi_results_list();
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifiscanclear (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) args, (void) env;
  WifiScanCount = 0;
  WifiScanEverStarted = false;
  WifiLastScanStartResult = 0;
  wifi_debugf("scan-clear\n");
  return nil;
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifiscanmap (object *args, object *env) {
  #if defined(ULISP_WIFI)
  object *function = first(args);
  size_t n = WifiScanCount;
  object *head = cons(NULL, NULL);
  protect(head);
  object *tail = head;
  for (int i = 0; i < n; i++) {
    WifiScanResult &r = WifiScanResults[i];
    object *ssid = lispstring((char *)r.ssid);
    protect(ssid);
    object *callargs = cons(ssid, cons(number(r.rssi), cons(number(r.security), cons(number(r.channel), nil))));
    protect(callargs);
    object *value = apply(function, callargs, env);
    unprotect(); // callargs
    unprotect(); // ssid
    protect(value);
    object *cell = cons(value, NULL);
    unprotect(); // value
    cdr(tail) = cell;
    tail = cell;
  }
  object *result = cdr(head);
  unprotect(); // head
  wifi_clear_reader_pushback("wifi-scan-map-return");
  return result;
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifistatus (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) args, (void) env;
  return number(cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA));
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

object *fn_wifidebug (object *args, object *env) {
  #if defined(ULISP_WIFI)
  (void) args, (void) env;
  netif *sta = &cyw43_state.netif[CYW43_ITF_STA];
  int wifi_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
  int tcpip_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
  int32_t rssi = 0;
  int rssi_err = cyw43_wifi_get_rssi(&cyw43_state, &rssi);
#if LWIP_DHCP
  dhcp *dhcp_state = netif_dhcp_data(sta);
  int dhcp_state_num = dhcp_state ? dhcp_state->state : -1;
  int dhcp_tries = dhcp_state ? dhcp_state->tries : -1;
#else
  int dhcp_state_num = -1;
  int dhcp_tries = -1;
#endif
  object *result = nil;
  result = alist_string("rssi", rssi_err == 0 ? number(rssi) : nil, result);
  result = alist_string("netmask", ip4_to_lisp(netif_ip4_netmask(sta)), result);
  result = alist_string("gateway", ip4_to_lisp(netif_ip4_gw(sta)), result);
  result = alist_string("local-ip", ip4_to_lisp(netif_ip4_addr(sta)), result);
  result = alist_string("dhcp-tries", number(dhcp_tries), result);
  result = alist_string("dhcp-state", lispstring((char *)wifi_dhcp_state_name(dhcp_state_num)), result);
  result = alist_string("dhcp-state-code", number(dhcp_state_num), result);
  result = alist_string("tcpip-status", lispstring((char *)wifi_link_status_name(tcpip_status)), result);
  result = alist_string("tcpip-status-code", number(tcpip_status), result);
  result = alist_string("wifi-status", lispstring((char *)wifi_link_status_name(wifi_status)), result);
  result = alist_string("wifi-status-code", number(wifi_status), result);
  return result;
  #else
  (void) args, (void) env;
  error2("not supported");
  return nil;
  #endif
}

