#pragma once

#include <libagent.hpp>

inline bool check_env(JuiceAgent::Agent& agent) {
    if (agent.get_jvm() == nullptr) {
        PLOGE << "Failed to get JavaVM instance";
        return false;
    }
    if (agent.get_jvmti() == nullptr) {
        PLOGE << "Failed to get JVMTI environment";
        return false;
    }
    return true;
}