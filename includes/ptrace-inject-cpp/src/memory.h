#pragma once

#include <sys/types.h>

#include <cstddef>
#include <cstdint>

// Open /proc/<pid>/mem for reading and writing.
// The process must already be ptrace-stopped.
int open_target_memory(pid_t pid);

void close_target_memory(int fd);

void read_memory(int fd, std::uint64_t address, void* buffer, std::size_t length);

void write_memory(int fd, std::uint64_t address, const void* buffer,
                  std::size_t length);
