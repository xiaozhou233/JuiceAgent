/*
 * ptrace_inject - C ABI for injecting shared libraries with ptrace.
 *
 * Part of ptrace-inject-cpp, a C++ port of the original Rust project:
 * https://github.com/Artemis21/ptrace-inject
 */

#ifndef PTRACE_INJECT_H
#define PTRACE_INJECT_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PTRACE_INJECT_VERSION_MAJOR 1
#define PTRACE_INJECT_VERSION_MINOR 0
#define PTRACE_INJECT_VERSION_PATCH 0
#define PTRACE_INJECT_VERSION_STRING "1.0.0"

#if defined(__GNUC__) || defined(__clang__)
#define PTRACE_INJECT_API __attribute__((visibility("default")))
#else
#define PTRACE_INJECT_API
#endif

/* Return values. */
enum ptrace_inject_result {
  PTRACE_INJECT_OK = 0,
  PTRACE_INJECT_ERROR = -1
};

/* Log levels passed to ptrace_inject_log_fn. */
enum ptrace_inject_log_level {
  PTRACE_INJECT_LOG_DEBUG = 0,
  PTRACE_INJECT_LOG_INFO = 1,
  PTRACE_INJECT_LOG_WARN = 2,
  PTRACE_INJECT_LOG_ERROR = 3
};

/*
 * Optional logging callback.
 *
 * `message` is only valid for the duration of the call.  Pass the same
 * `ptrace_inject_options` to every injection; the callback runs on the
 * thread that called ptrace_inject_*().
 */
typedef void (*ptrace_inject_log_fn)(int level, const char *message,
                                     void *user_data);

typedef struct ptrace_inject_options {
  ptrace_inject_log_fn log_fn; /* may be NULL */
  void *log_user_data;         /* passed back to log_fn */
} ptrace_inject_options;

/*
 * Inject `library_path` into the process `pid`.
 *
 * `options` may be NULL (silent).  `error` may be NULL; when non-NULL and
 * `error_size > 0`, it receives a NUL-terminated error message on failure and
 * an empty string on success.
 *
 * Returns PTRACE_INJECT_OK on success, PTRACE_INJECT_ERROR on failure.
 *
 * The call is synchronous and blocks until the library has been dlopen()ed
 * (or the call failed).  It is safe to call from multiple threads at the same
 * time as long as the targets are different.
 */
PTRACE_INJECT_API int
ptrace_inject_by_pid(pid_t pid, const char *library_path,
                     const ptrace_inject_options *options, char *error,
                     size_t error_size);

/*
 * Same as ptrace_inject_by_pid(), but finds the target by executable path or
 * file name.  `resolved_pid` may be NULL; when non-NULL it receives the PID
 * that was found (only on success).
 */
PTRACE_INJECT_API int
ptrace_inject_by_name(const char *process_name, const char *library_path,
                      const ptrace_inject_options *options,
                      pid_t *resolved_pid, char *error, size_t error_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PTRACE_INJECT_H */
