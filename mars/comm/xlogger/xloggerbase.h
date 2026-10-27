// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/*
 ============================================================================
 ============================================================================
 */

#ifndef XLOGGERBASE_H_
#define XLOGGERBASE_H_

#include <stdarg.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kLevelAll = 0,
    kLevelVerbose = 0,
    kLevelDebug,  // Detailed information on the flow through the system.
    kLevelInfo,   // Interesting runtime events (startup/shutdown), should be conservative and keep to a minimum.
    kLevelWarn,   // Other runtime situations that are undesirable or unexpected, but not necessarily "wrong".
    kLevelError,  // Other runtime errors or unexpected conditions.
    kLevelFatal,  // Severe errors that cause premature termination.
    kLevelNone,   // Special level used to disable all log messages.
} TLogLevel;

typedef struct XLoggerInfo_t {
    TLogLevel level;
    const char* tag;
    const char* filename;
    const char* func_name;
    int line;

    struct timeval timeval;
    intmax_t pid;
    intmax_t tid;
    intmax_t maintid;
    int traceLog;
} XLoggerInfo;
// If there are fewer initializers in a brace-enclosed list than there are elements or members of an aggregate,
// or fewer characters in a string literal used to initialize an array of known size than there are elements
// in the array, the remainder of the aggregate shall be initialized implicitly
// the same as objects that have static storage duration.
#define XLOGGER_INFO_INITIALIZER \
    { (TLogLevel)0 }

extern intmax_t xlogger_pid();
extern intmax_t xlogger_tid();
extern intmax_t xlogger_maintid();
typedef void (*xlogger_appender_t)(const XLoggerInfo* _info, const char* _log);
extern const char* xlogger_dump(const void* _dumpbuffer, size_t _len);
extern const char* xlogger_memory_dump(const void* _dumpbuffer,
                                       size_t _len);  // same as xlogger_dump, but don't write dumpbuffer to file.

TLogLevel xlogger_Level();
void xlogger_SetLevel(TLogLevel _level);
int xlogger_IsEnabledFor(TLogLevel _level);
xlogger_appender_t xlogger_SetAppender(xlogger_appender_t _appender);

typedef int (*xlogger_filter_t)(XLoggerInfo* _info, const char* _log);
void xlogger_SetFilter(xlogger_filter_t _filter);
xlogger_filter_t xlogger_GetFilter();

// no level filter
#ifdef __GNUC__
__attribute__((__format__(printf, 3, 4)))
#endif
void        xlogger_AssertP(const XLoggerInfo* _info, const char* _expression, const char* _format, ...);
void xlogger_Assert(const XLoggerInfo* _info, const char* _expression, const char* _log);
#ifdef __GNUC__
__attribute__((__format__(printf, 2, 0)))
#endif
void        xlogger_VPrint(const XLoggerInfo* _info, const char* _format, va_list _list);
#ifdef __GNUC__
__attribute__((__format__(printf, 2, 3)))
#endif
void        xlogger_Print(const XLoggerInfo* _info, const char* _format, ...);
// Writes preformatted text to the global appender without printf processing.
// Metadata storage is supplied by the caller; this entry point does not
// allocate XLoggerInfo. A caller reusing storage must update per-entry fields
// and keep it intact throughout the call, including any reentrant log hooks.
void xlogger_Write(const XLoggerInfo* _info, const char* _log);

// FFI writers bind an existing appender once and dispatch entries by pointer.
// Each entry supplies its own tag. Writers do not create or own log instances. The native
// owner MUST keep the appender open until every writer using it is released.
// Release frees only the binding; it never flushes/closes the native appender.
// Each writer has one serialized owner (for example, one Dart isolate).
// Synchronous reentrant writes are supported; concurrent writes or release
// require external serialization. Separate writers can bind the same appender.
// Release is forbidden until all calls on this writer have returned, including
// calls made from synchronous hooks. Hooks must not throw across this C ABI.
typedef struct XLoggerWriter XLoggerWriter;

#if defined(_WIN32)
#define XLOGGER_WRITER_API __declspec(dllexport)
#elif defined(__GNUC__)
#define XLOGGER_WRITER_API __attribute__((visibility("default"))) __attribute__((used))
#else
#define XLOGGER_WRITER_API
#endif

enum XLoggerWriterResult {
    XLOGGER_WRITER_OK = 0,  // Accepted by xlog, not a durability guarantee.
    XLOGGER_WRITER_FILTERED = 1,
    XLOGGER_WRITER_INVALID_ARGUMENT = 2
};

// NULL prefix explicitly selects the global appender. Missing/empty named
// prefixes return NULL and never fall back to the global appender.
XLOGGER_WRITER_API XLoggerWriter* xlogger_writer_bind_existing(const char* prefix);
// Synchronously consumes tag and message as NUL-terminated UTF-8 strings. Both
// buffers must remain valid until the call returns, including reentrant hooks.
// They are borrowed for this call only; changing tag does not bind a new writer.
// The adapter performs no instance lookup,
// locking, refcounting or heap allocation per write. Metadata and fixed fields
// are cached at binding; tag, level, timestamp and current thread ID vary per entry.
// A synchronous reentrant write uses separate stack metadata.
// The xlog backend itself may allocate, lock or block.
XLOGGER_WRITER_API int32_t xlogger_writer_write(XLoggerWriter* writer, int32_t level, const char* tag, const char* message);
/**
 * Queries the current severity threshold of the appender bound to writer.
 * Uses the bound pointer directly, without an instance-name lookup, message
 * encoding, heap allocation, or a log write. An optional early check can avoid
 * expensive message construction; ordinary callers can just call write(),
 * which performs its own level check within the same write call.
 *
 * @param writer A live binding returned by bind_existing(). A NULL binding
 *               returns 0. A released binding must never be passed here.
 * @param level  A TLogLevel from kLevelVerbose through kLevelFatal, inclusive.
 * @return 1 if that severity is currently enabled, otherwise 0. Invalid levels
 *         (including kLevelNone) also return 0. This checks severity only;
 *         downstream xlog filters may still reject the entry. The result is a
 *         snapshot, not a reservation: a later write checks the level again.
 */
XLOGGER_WRITER_API int32_t xlogger_writer_is_enabled(const XLoggerWriter* writer, int32_t level);
// Destroys only the binding allocated by bind_existing().
// NULL is allowed. Every non-NULL binding must be released exactly once through
// this function, never through free() or a caller's delete. The pointer becomes
// invalid on return; a second release or any later access is undefined behavior.
// This does not flush, close or delete the bound native log instance.
XLOGGER_WRITER_API void xlogger_writer_release(XLoggerWriter* writer);

#ifdef __cplusplus
}
#endif

#endif
