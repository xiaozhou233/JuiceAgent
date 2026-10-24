#pragma once

#include <string>

#include "ptrace_inject.h"

// Internal representation of ptrace_inject_options.
struct LogSink {
  ptrace_inject_log_fn fn = nullptr;
  void* user_data = nullptr;
};

// All of these do nothing when `sink` is null or `sink->fn` is null.
void log_message(const LogSink* sink, int level, const std::string& message);
void log_debug(const LogSink* sink, const std::string& message);
void log_info(const LogSink* sink, const std::string& message);
void log_warn(const LogSink* sink, const std::string& message);
