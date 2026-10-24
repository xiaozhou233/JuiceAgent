#include "ptrace_inject.h"

#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "inject.h"
#include "log.h"
#include "process_lookup.h"
#include "ptrace_utils.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void write_error(char* error, std::size_t error_size,
                        const std::string& message) {
  if (error != nullptr && error_size > 0) {
    std::snprintf(error, error_size, "%s", message.c_str());
  }
}

static LogSink make_log_sink(const ptrace_inject_options* options) {
  LogSink sink;
  if (options != nullptr) {
    sink.fn = options->log_fn;
    sink.user_data = options->log_user_data;
  }
  return sink;
}

// Shared implementation for both entry points.  `pid` is already resolved.
static int inject_impl(pid_t pid, const char* library_path,
                       const ptrace_inject_options* options, char* error,
                       std::size_t error_size) {
  if (library_path == nullptr || library_path[0] == '\0') {
    write_error(error, error_size, "library path is null or empty");
    return PTRACE_INJECT_ERROR;
  }
  if (pid <= 1) {
    write_error(error, error_size,
                "refusing to attach to pid " + std::to_string(pid));
    return PTRACE_INJECT_ERROR;
  }
  if (pid == getpid()) {
    write_error(error, error_size, "refusing to inject into the injector itself");
    return PTRACE_INJECT_ERROR;
  }

  const LogSink log = make_log_sink(options);
  std::vector<pid_t> attached;

  try {
    attach_all_threads(pid, attached, &log);

    // Prefer the main thread, but any stopped thread works.
    pid_t tracee = pid;
    if (std::find(attached.begin(), attached.end(), tracee) ==
        attached.end()) {
      tracee = attached.front();
    }

    inject_process(pid, tracee, attached, library_path, &log);
    detach_all_threads(attached, -1, 0, &log);

    write_error(error, error_size, "");
    log_info(&log, "injected " + std::string(library_path) + " into pid " +
                       std::to_string(pid));
    return PTRACE_INJECT_OK;
  } catch (const TraceeSignalError& caught) {
    // Give the pending signal back to the thread instead of swallowing it.
    detach_all_threads(attached, caught.tid, caught.signal, &log);
    write_error(error, error_size, caught.what());
    return PTRACE_INJECT_ERROR;
  } catch (const std::exception& caught) {
    detach_all_threads(attached, -1, 0, &log);
    write_error(error, error_size, caught.what());
    return PTRACE_INJECT_ERROR;
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

extern "C" int ptrace_inject_by_pid(pid_t pid, const char* library_path,
                                    const ptrace_inject_options* options,
                                    char* error, std::size_t error_size) {
  return inject_impl(pid, library_path, options, error, error_size);
}

extern "C" int ptrace_inject_by_name(const char* process_name,
                                     const char* library_path,
                                     const ptrace_inject_options* options,
                                     pid_t* resolved_pid, char* error,
                                     std::size_t error_size) {
  if (process_name == nullptr || process_name[0] == '\0') {
    write_error(error, error_size, "process name is null or empty");
    return PTRACE_INJECT_ERROR;
  }

  const LogSink log = make_log_sink(options);

  pid_t pid = -1;
  try {
    pid = find_pid_by_name(process_name);
  } catch (const std::exception& caught) {
    write_error(error, error_size, caught.what());
    return PTRACE_INJECT_ERROR;
  }

  if (pid == -1) {
    write_error(error, error_size,
                "could not find process named '" + std::string(process_name) +
                    "'");
    return PTRACE_INJECT_ERROR;
  }

  if (resolved_pid != nullptr) {
    *resolved_pid = pid;
  }
  log_info(&log, "found pid " + std::to_string(pid) + " for name '" +
                     std::string(process_name) + "'");

  return inject_impl(pid, library_path, options, error, error_size);
}
