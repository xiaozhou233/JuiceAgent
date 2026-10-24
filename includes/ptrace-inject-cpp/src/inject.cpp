#include "inject.h"

#include <dlfcn.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "log.h"
#include "maps.h"
#include "memory.h"
#include "ptrace_utils.h"
#include "symbols.h"
#include "util.h"

// ---------------------------------------------------------------------------
// Choosing where to patch
// ---------------------------------------------------------------------------

static bool overlaps(std::uint64_t address, std::uint64_t other,
                     std::uint64_t length) {
  return address + length > other && address < other + length;
}

// Pick a page-aligned spot in a private, file-backed executable mapping that
// no stopped thread is currently sitting in.
static std::uint64_t find_patch_site(pid_t pid,
                                     const std::vector<pid_t>& threads) {
  std::vector<std::uint64_t> instruction_pointers;
  instruction_pointers.reserve(threads.size());
  for (const pid_t tid : threads) {
    instruction_pointers.push_back(get_registers(tid).rip);
  }

  for (const Mapping& map : read_maps(pid)) {
    if (!is_executable(map) || map.perms.size() < 4 || map.perms[3] != 'p') {
      continue;
    }
    if (map.path.empty() || map.path[0] == '[') {
      continue;  // skip [vdso], [vsyscall], anonymous mappings
    }

    for (std::uint64_t address = map.start;
         address + kShellcode.size() <= map.end; address += 0x1000) {
      bool busy = false;
      for (const std::uint64_t rip : instruction_pointers) {
        if (overlaps(address, rip, kShellcode.size())) {
          busy = true;
          break;
        }
      }
      if (!busy) {
        return address;
      }
    }
  }

  throw std::runtime_error("could not find a safe executable place to patch");
}

// Use the target's own stack, but keep clear of the red zone below rsp.  If
// that region is not writable, fall back to the aligned rsp.
static std::uint64_t choose_stack_pointer(pid_t pid, std::uint64_t saved_rsp) {
  if (saved_rsp > kStackMargin) {
    const std::uint64_t candidate =
        (saved_rsp - kStackMargin) & ~std::uint64_t{0xf};
    if (address_is_writable(pid, candidate)) {
      return candidate;
    }
  }

  const std::uint64_t fallback = saved_rsp & ~std::uint64_t{0xf};
  if (address_is_writable(pid, fallback)) {
    return fallback;
  }

  throw std::runtime_error(
      "target stack is not writable; refusing to continue");
}

// ---------------------------------------------------------------------------
// Remote calls
// ---------------------------------------------------------------------------

static std::uint64_t call_in_tracee(pid_t tid,
                                    std::uint64_t shellcode_address,
                                    std::uint64_t stack_pointer,
                                    const user_regs_struct& saved_registers,
                                    std::uint64_t function, std::uint64_t rdi,
                                    std::uint64_t rsi) {
  user_regs_struct regs = saved_registers;

  // +2 skips the two NOPs.  If resuming rewinds RIP by 2, execution starts at
  // the NOPs instead, and the nop slide still reaches "call r9".
  regs.rip = shellcode_address + 2;
  regs.r9 = function;
  regs.rdi = rdi;
  regs.rsi = rsi;
  // rsp must be 16-byte aligned before the call (x86-64 SysV ABI).
  regs.rsp = stack_pointer;

  set_registers(tid, regs);
  continue_tracee(tid);
  wait_for_trap(tid, shellcode_address + kShellcode.size());

  return get_registers(tid).rax;
}

// ---------------------------------------------------------------------------
// Top-level injection
// ---------------------------------------------------------------------------

// Validate the library path before anything in the target is modified.
static std::string resolve_library_path(const std::string& library) {
  std::string path;
  try {
    path = std::filesystem::canonical(library).string();
  } catch (const std::filesystem::filesystem_error& error) {
    throw std::runtime_error("cannot resolve library path '" + library +
                             "': " + error.what());
  }
  if (!std::filesystem::is_regular_file(path)) {
    throw std::runtime_error("'" + path + "' is not a regular file");
  }
  path.push_back('\0');
  return path;
}

void inject_process(pid_t pid, pid_t tracee,
                    const std::vector<pid_t>& threads,
                    const std::string& library, const LogSink* log) {
  // Validate the library and the remote function addresses before patching.
  const std::string path = resolve_library_path(library);
  const LibcAddresses libc = find_libc_addresses(pid);
  const std::uint64_t shellcode_address = find_patch_site(pid, threads);

  log_debug(log, "remote malloc: 0x" + hex_string(libc.malloc) +
                     ", dlopen: 0x" + hex_string(libc.dlopen) +
                     ", free: 0x" + hex_string(libc.free));

  const int memory_fd = open_target_memory(pid);

  user_regs_struct saved_registers{};
  std::array<std::uint8_t, kShellcode.size()> saved_bytes{};
  std::uint64_t stack_pointer = 0;
  bool patched = false;

  try {
    saved_registers = get_registers(tracee);
    stack_pointer = choose_stack_pointer(pid, saved_registers.rsp);
    read_memory(memory_fd, shellcode_address, saved_bytes.data(),
                saved_bytes.size());

    patched = true;
    write_memory(memory_fd, shellcode_address, kShellcode.data(),
                 kShellcode.size());
    log_debug(log, "patched shellcode at 0x" + hex_string(shellcode_address));

    // 1. malloc() space in the target for the path.
    const std::uint64_t remote_path =
        call_in_tracee(tracee, shellcode_address, stack_pointer,
                       saved_registers, libc.malloc, path.size(), 0);
    if (remote_path == 0) {
      throw std::runtime_error("malloc inside tracee returned NULL");
    }
    log_debug(log, "remote malloc(" + std::to_string(path.size()) + ") = 0x" +
                       hex_string(remote_path));
    write_memory(memory_fd, remote_path, path.data(), path.size());

    // 2. dlopen(path, RTLD_LAZY).
    const std::uint64_t handle =
        call_in_tracee(tracee, shellcode_address, stack_pointer,
                       saved_registers, libc.dlopen, remote_path, RTLD_LAZY);
    if (handle == 0) {
      throw std::runtime_error("dlopen inside tracee returned NULL");
    }
    log_debug(log, "remote dlopen(\"" + library + "\") = 0x" +
                       hex_string(handle));

    // 3. free() the path buffer.  Failure is not fatal.
    call_in_tracee(tracee, shellcode_address, stack_pointer, saved_registers,
                   libc.free, remote_path, 0);
    log_debug(log, "remote free() done");
  } catch (...) {
    // Best effort: write both the bytes and the registers back.
    if (patched) {
      try {
        write_memory(memory_fd, shellcode_address, saved_bytes.data(),
                     saved_bytes.size());
      } catch (const std::exception& error) {
        log_warn(log, "failed to restore code: " + std::string(error.what()));
      }
      try {
        set_registers(tracee, saved_registers);
      } catch (const std::exception& error) {
        log_warn(log,
                 "failed to restore registers: " + std::string(error.what()));
      }
    }
    close_target_memory(memory_fd);
    throw;
  }

  try {
    write_memory(memory_fd, shellcode_address, saved_bytes.data(),
                 saved_bytes.size());
    set_registers(tracee, saved_registers);
  } catch (...) {
    close_target_memory(memory_fd);
    throw;
  }
  close_target_memory(memory_fd);
  log_info(log, "restored original code and registers");
}
