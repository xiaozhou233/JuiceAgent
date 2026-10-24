#include "log.h"

void log_message(const LogSink* sink, int level, const std::string& message) {
  if (sink != nullptr && sink->fn != nullptr) {
    sink->fn(level, message.c_str(), sink->user_data);
  }
}

void log_debug(const LogSink* sink, const std::string& message) {
  log_message(sink, PTRACE_INJECT_LOG_DEBUG, message);
}

void log_info(const LogSink* sink, const std::string& message) {
  log_message(sink, PTRACE_INJECT_LOG_INFO, message);
}

void log_warn(const LogSink* sink, const std::string& message) {
  log_message(sink, PTRACE_INJECT_LOG_WARN, message);
}
