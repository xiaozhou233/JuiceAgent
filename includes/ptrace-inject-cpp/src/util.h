#pragma once

#include <cstdint>
#include <string>

// Throw std::runtime_error with strerror(errno) appended.
[[noreturn]] void throw_errno(const std::string& what);

bool all_digits(const std::string& text);

// Remove the " (deleted)" suffix that /proc/<pid>/maps adds to unlinked files.
std::string strip_deleted(std::string path);

std::string trim(std::string text);

std::string hex_string(std::uint64_t value);
