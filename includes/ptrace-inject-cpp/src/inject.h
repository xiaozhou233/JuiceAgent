#pragma once

#include <sys/types.h>

#include <string>
#include <vector>

#include "log.h"

// Load `library` into the already-stopped target process.
//
// `pid` is the target process, `tracee` is the specific (stopped) thread used
// to execute the remote calls, and `threads` is the full list of stopped
// threads (used to avoid patching code a thread is currently executing).
//
// Throws on failure.  Any code/register modification is rolled back before the
// exception leaves this function.
void inject_process(pid_t pid, pid_t tracee,
                    const std::vector<pid_t>& threads,
                    const std::string& library, const LogSink* log);
