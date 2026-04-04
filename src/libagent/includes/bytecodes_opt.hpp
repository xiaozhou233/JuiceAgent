#pragma once

#include "event/event_type.hpp"

namespace JuiceAgent::Bytecodes {
    void register_bytecodes();

    void capture_bytecodes(const EventClassFileLoadHook& e);
    void patch_bytecodes(const EventClassFileLoadHook& e);
}