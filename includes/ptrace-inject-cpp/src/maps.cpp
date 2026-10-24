#include "maps.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "util.h"

bool is_executable(const Mapping& map) {
  return map.perms.size() >= 3 && map.perms[2] == 'x';
}

bool is_writable(const Mapping& map) {
  return map.perms.size() >= 2 && map.perms[1] == 'w';
}

std::vector<Mapping> read_maps(pid_t pid) {
  const std::string file = "/proc/" + std::to_string(pid) + "/maps";
  std::ifstream in(file);
  if (!in) {
    throw std::runtime_error("cannot open " + file);
  }

  std::vector<Mapping> maps;
  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string range;
    std::string offset;
    std::string dev;
    std::string inode;

    Mapping map;
    if (!(iss >> range >> map.perms >> offset >> dev >> inode)) {
      continue;
    }

    const auto dash = range.find('-');
    if (dash == std::string::npos) {
      continue;
    }
    try {
      map.start = std::stoull(range.substr(0, dash), nullptr, 16);
      map.end = std::stoull(range.substr(dash + 1), nullptr, 16);
      map.inode = std::stoull(inode, nullptr, 10);
    } catch (const std::exception&) {
      continue;
    }
    map.dev = dev;

    // The file name may contain spaces, so take the rest of the line.
    std::string rest;
    std::getline(iss, rest);
    map.path = trim(rest);

    maps.push_back(map);
  }
  return maps;
}

std::string file_name_of(const std::string& path) {
  return std::filesystem::path(strip_deleted(path)).filename().string();
}

bool address_is_executable(pid_t pid, std::uint64_t address) {
  for (const Mapping& map : read_maps(pid)) {
    if (address >= map.start && address < map.end) {
      return is_executable(map);
    }
  }
  return false;
}

bool address_is_writable(pid_t pid, std::uint64_t address) {
  for (const Mapping& map : read_maps(pid)) {
    if (address >= map.start && address < map.end) {
      return is_writable(map);
    }
  }
  return false;
}

ObjectInfo find_object(pid_t pid, const std::string& object_name) {
  for (const Mapping& map : read_maps(pid)) {
    if (file_name_of(map.path) == object_name) {
      return ObjectInfo{map.start, map.dev, map.inode};
    }
  }
  throw std::runtime_error("could not find " + object_name + " in process " +
                           std::to_string(pid));
}
