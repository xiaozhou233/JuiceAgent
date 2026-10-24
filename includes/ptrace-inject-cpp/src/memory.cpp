#include "memory.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "util.h"

int open_target_memory(pid_t pid) {
  const std::string path = "/proc/" + std::to_string(pid) + "/mem";
  const int fd = open(path.c_str(), O_RDWR | O_CLOEXEC);
  if (fd == -1) {
    throw_errno("open " + path);
  }
  return fd;
}

void close_target_memory(int fd) {
  if (fd >= 0) {
    close(fd);
  }
}

void read_memory(int fd, std::uint64_t address, void* buffer,
                 std::size_t length) {
  auto* out = static_cast<std::uint8_t*>(buffer);
  std::size_t done = 0;
  while (done < length) {
    const ssize_t n =
        pread(fd, out + done, length - done, static_cast<off_t>(address + done));
    if (n == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw_errno("pread from target memory at 0x" +
                  hex_string(address + done));
    }
    if (n == 0) {
      throw std::runtime_error("short read from target memory at 0x" +
                               hex_string(address + done));
    }
    done += static_cast<std::size_t>(n);
  }
}

void write_memory(int fd, std::uint64_t address, const void* buffer,
                  std::size_t length) {
  const auto* in = static_cast<const std::uint8_t*>(buffer);
  std::size_t done = 0;
  while (done < length) {
    const ssize_t n = pwrite(fd, in + done, length - done,
                             static_cast<off_t>(address + done));
    if (n == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw_errno("pwrite to target memory at 0x" + hex_string(address + done));
    }
    if (n == 0) {
      throw std::runtime_error("short write to target memory at 0x" +
                               hex_string(address + done));
    }
    done += static_cast<std::size_t>(n);
  }
}
