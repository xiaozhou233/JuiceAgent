#pragma once

#include <sys/types.h>
#include <sys/user.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "log.h"

// Thrown when the tracee stops with a signal that is not our int3 SIGTRAP.
// main() uses it to deliver that signal back to the target on detach.
struct TraceeSignalError : public std::runtime_error {
  TraceeSignalError(pid_t tracee, int signo, const std::string& what)
      : std::runtime_error(what), tid(tracee), signal(signo) {}

  pid_t tid;
  int signal;
};

user_regs_struct get_registers(pid_t tid);
void set_registers(pid_t tid, const user_regs_struct& regs);

void continue_tracee(pid_t tid);

// Wait for our int3.  expected_rip must be kShellcodeAddress + kShellcode.size().
void wait_for_trap(pid_t tid, std::uint64_t expected_rip);

// Stop every thread of `pid`; on return all of them are ptrace-stopped.
void attach_all_threads(pid_t pid, std::vector<pid_t>& attached,
                        const LogSink* log);

// Detach all threads.  If signal != 0, deliver it to signal_tid on detach.
void detach_all_threads(std::vector<pid_t>& attached, pid_t signal_tid,
                        int signal, const LogSink* log);
