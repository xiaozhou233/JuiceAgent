#pragma once

#include <sys/types.h>

#include <cstdint>
#include <string>
#include <vector>

// One line of /proc/<pid>/maps.
struct Mapping {
  std::uint64_t start = 0;
  std::uint64_t end = 0;
  std::string dev;
  std::uint64_t inode = 0;
  std::string perms;
  std::string path;
};

// Load base and file identity of a shared object in a given process.
struct ObjectInfo {
  std::uint64_t base = 0;
  std::string dev;
  std::uint64_t inode = 0;
};

std::vector<Mapping> read_maps(pid_t pid);

bool is_executable(const Mapping& map);
bool is_writable(const Mapping& map);

std::string file_name_of(const std::string& path);

bool address_is_executable(pid_t pid, std::uint64_t address);
bool address_is_writable(pid_t pid, std::uint64_t address);

// Find the first mapping (file offset 0, i.e. the load base) of a shared
// object identified by its file name.
ObjectInfo find_object(pid_t pid, const std::string& object_name);
