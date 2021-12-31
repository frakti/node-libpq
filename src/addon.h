#ifndef NODE_LIBPQ_ADDON
#define NODE_LIBPQ_ADDON

#include <uv.h>
#include <napi.h>
#include <libpq-fe.h>
#include <pg_config.h>

#if PG_VERSION_NUM > 90000
#define ESCAPE_SUPPORTED
#endif

#if PG_VERSION_NUM >= 93000
#define MORE_ERROR_FIELDS_SUPPORTED
#endif

#include "connection.h"
#include "connect-async-worker.h"

//#define LOG(msg) fprintf(stderr, "%s\n", msg);
//#define TRACEF(format, arg) fprintf(stderr, format, arg);

#define LOG(msg) ;
#define TRACEF(format, arg) ;

#define TRACE(msg) LOG(msg);
#define NODE_THIS() Napi::ObjectWrap<Connection>::Unwrap(info.This().ToObject());

// info.This().Value()
#endif
