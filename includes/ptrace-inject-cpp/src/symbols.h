#pragma once

#include <sys/types.h>

#include <cstdint>

// Addresses of the libc functions we need, already relocated into the target.
struct LibcAddresses {
  std::uint64_t malloc = 0;
  std::uint64_t dlopen = 0;
  std::uint64_t free = 0;
};

// Resolve malloc/dlopen/free locally and translate their addresses into the
// address space of `target_pid`.
//
// Refuses to continue (throws) if the injector and the target use different
// builds of the owning shared object, or if a relocated address is not
// executable there.
LibcAddresses find_libc_addresses(pid_t target_pid);
