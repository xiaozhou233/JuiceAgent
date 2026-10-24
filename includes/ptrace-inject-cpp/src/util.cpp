#include "util.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>

void throw_errno(const std::string& what) {
  const int err = errno;
  throw std::runtime_error(what + ": " + std::strerror(err));
}

bool all_digits(const std::string& text) {
  return !text.empty() &&
         std::all_of(text.begin(), text.end(), [](unsigned char c) {
           return std::isdigit(c) != 0;
         });
}

std::string strip_deleted(std::string path) {
  static const std::string suffix = " (deleted)";
  if (path.size() > suffix.size() &&
      path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0) {
    path.resize(path.size() - suffix.size());
  }
  return path;
}

std::string trim(std::string text) {
  const auto begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = text.find_last_not_of(" \t\r\n");
  return text.substr(begin, end - begin + 1);
}

std::string hex_string(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << value;
  return out.str();
}
