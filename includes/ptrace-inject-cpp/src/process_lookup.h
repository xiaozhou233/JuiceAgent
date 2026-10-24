#pragma once

#include <sys/types.h>

#include <string>

// Return the PID of the first process whose executable path or file name is
// `name`, or -1 if no process matches.
pid_t find_pid_by_name(const std::string& name);
