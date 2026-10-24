#pragma once

#include <array>
#include <cstdint>

// The 6-byte shellcode patched into the tracee:
//
//   90 90          nop; nop
//   41 ff d1       call r9
//   cc             int3
//
// The tracer sets r9 to the function to call, rdi/rsi to its arguments, and
// rip to kShellcode + 2 (the nop slide covers a possible RIP rewind).
inline constexpr std::array<std::uint8_t, 6> kShellcode = {
    0x90, 0x90,
    0x41, 0xff, 0xd1,
    0xcc,
};

// Keep this much free space below the target's rsp so we do not clobber the
// 128-byte red zone of the function that was interrupted.
inline constexpr std::uint64_t kStackMargin = 256;

// How many times to re-scan /proc/<pid>/task while stopping threads.
inline constexpr int kMaxAttachRounds = 8;
