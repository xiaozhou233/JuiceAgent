#include <limits.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>

#include "ptrace_inject.h"

static void print_log(int level, const char* message, void* /*user_data*/) {
  const char* prefix = "[*]";
  switch (level) {
    case PTRACE_INJECT_LOG_DEBUG:
      prefix = "[debug]";
      break;
    case PTRACE_INJECT_LOG_INFO:
      prefix = "[*]";
      break;
    case PTRACE_INJECT_LOG_WARN:
      prefix = "[!]";
      break;
    case PTRACE_INJECT_LOG_ERROR:
      prefix = "[error]";
      break;
    default:
      break;
  }
  std::fprintf(stderr, "%s %s\n", prefix, message);
}

static void usage(const char* program) {
  std::fprintf(stderr,
               "Usage: %s (-p PID | -n NAME) <library.so>\n"
               "\n"
               "Inject a shared library into a running x86-64 Linux process.\n"
               "\n"
               "  -p PID    attach to this PID\n"
               "  -n NAME   attach to the first process whose executable name\n"
               "            or path matches NAME\n"
               "  -h        show this help\n"
               "\n"
               "Requires the same uid and CAP_SYS_PTRACE, or a suitable\n"
               "kernel.yama.ptrace_scope setting.\n"
               "\n"
               "Example: %s -p 123456 ./libloader.so\n"
               "\n"
               "Default runtime directory: ${HOME}/.juiceagent/ \n"
               "See README for more information.\n",
               program, program);
}

int main(int argc, char** argv) {
  pid_t pid = -1;
  std::string name;

  int option;
  while ((option = getopt(argc, argv, "p:n:h")) != -1) {
    switch (option) {
      case 'p': {
        try {
          const long value = std::stol(optarg);
          if (value <= 0 || value > INT_MAX) {
            throw std::out_of_range("pid");
          }
          pid = static_cast<pid_t>(value);
        } catch (const std::exception&) {
          std::fprintf(stderr, "error: invalid pid '%s'\n", optarg);
          return 1;
        }
        break;
      }
      case 'n':
        name = optarg;
        break;
      case 'h':
        usage(argv[0]);
        return 0;
      default:
        usage(argv[0]);
        return 1;
    }
  }

  if (optind + 1 != argc) {
    usage(argv[0]);
    return 1;
  }
  const char* library = argv[optind];

  if ((pid == -1) == name.empty()) {
    std::fprintf(stderr, "error: specify exactly one of -p PID or -n NAME\n");
    usage(argv[0]);
    return 1;
  }

  ptrace_inject_options options{};
  options.log_fn = print_log;

  char error[512] = {};
  int result;
  if (!name.empty()) {
    result = ptrace_inject_by_name(name.c_str(), library, &options, nullptr,
                                   error, sizeof(error));
  } else {
    result = ptrace_inject_by_pid(pid, library, &options, error, sizeof(error));
  }

  if (result != PTRACE_INJECT_OK) {
    std::fprintf(stderr, "error: %s\n", error);
    return 1;
  }
  return 0;
}
