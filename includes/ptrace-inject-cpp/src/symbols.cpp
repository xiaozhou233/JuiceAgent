#include "symbols.h"

#include <dlfcn.h>
#include <unistd.h>

#include <cstdint>
#include <stdexcept>
#include <string>

#include "maps.h"
#include "util.h"

struct SymbolAddress {
  const char* name;
  std::uint64_t address;
  std::string object;
};

static SymbolAddress resolve_symbol(const char* name) {
  dlerror();  // clear any previous error
  void* address = dlsym(RTLD_DEFAULT, name);
  if (const char* error = dlerror()) {
    throw std::runtime_error(std::string("dlsym(") + name + ") failed: " +
                             error);
  }

  Dl_info info{};
  if (dladdr(address, &info) == 0 || info.dli_fname == nullptr) {
    throw std::runtime_error(std::string("dladdr(") + name + ") failed");
  }

  return SymbolAddress{name, reinterpret_cast<std::uint64_t>(address),
                       file_name_of(info.dli_fname)};
}

// Translate one local symbol address into the target's address space.
//
// malloc/free are in libc.so.6; dlopen is in libc.so.6 on new glibc and in
// libdl.so.2 on old glibc.  dladdr() tells us which object owns the symbol.
static std::uint64_t relocate_symbol(const SymbolAddress& symbol,
                                     pid_t target_pid) {
  const ObjectInfo local = find_object(getpid(), symbol.object);
  const ObjectInfo target = find_object(target_pid, symbol.object);

  if (local.dev != target.dev || local.inode != target.inode) {
    throw std::runtime_error(
        std::string("injector and target use different builds of ") +
        symbol.object + "; refusing to guess the address of " + symbol.name);
  }

  const std::uint64_t address = symbol.address - local.base + target.base;
  if (!address_is_executable(target_pid, address)) {
    throw std::runtime_error(std::string("relocated ") + symbol.name +
                             " is not executable in the target; refusing to "
                             "continue");
  }
  return address;
}

LibcAddresses find_libc_addresses(pid_t target_pid) {
  return LibcAddresses{
      relocate_symbol(resolve_symbol("malloc"), target_pid),
      relocate_symbol(resolve_symbol("dlopen"), target_pid),
      relocate_symbol(resolve_symbol("free"), target_pid),
  };
}
