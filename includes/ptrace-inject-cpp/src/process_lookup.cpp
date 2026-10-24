#include "process_lookup.h"

#include <limits.h>
#include <unistd.h>

#include <filesystem>
#include <stdexcept>
#include <string>

#include "util.h"

pid_t find_pid_by_name(const std::string& name) {
  std::error_code ec;
  std::filesystem::directory_iterator iterator("/proc", ec);
  const std::filesystem::directory_iterator end;
  if (ec) {
    throw std::runtime_error("cannot open /proc: " + ec.message());
  }

  for (; iterator != end; iterator.increment(ec)) {
    if (ec) {
      break;
    }

    const std::string pid_text = iterator->path().filename().string();
    if (!all_digits(pid_text)) {
      continue;
    }

    char buffer[PATH_MAX + 1] = {};
    const ssize_t length =
        readlink((iterator->path() / "exe").c_str(), buffer, PATH_MAX);
    if (length <= 0) {
      continue;  // usually a permission problem
    }
    buffer[length] = '\0';

    const std::string executable = strip_deleted(buffer);
    const std::string executable_name =
        std::filesystem::path(executable).filename().string();
    if (executable_name == name || executable == name) {
      return static_cast<pid_t>(std::stoi(pid_text));
    }
  }
  return -1;
}
