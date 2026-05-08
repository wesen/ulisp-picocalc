// ulisp_http.h — Pico 2W cooperative HTTP server primitives.

#ifndef ULISP_HTTP_H
#define ULISP_HTTP_H

#include "ulisp_types.h"

object *fn_httpserverstart(object *args, object *env);
object *fn_httpserverstop(object *args, object *env);
object *fn_httpaccept(object *args, object *env);
object *fn_httprespond(object *args, object *env);
object *fn_urldecode(object *args, object *env);
object *fn_htmlescape(object *args, object *env);

#endif // ULISP_HTTP_H
