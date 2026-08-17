#pragma once

#include <services/base.hpp>
#include <services/service_manager.hpp>
#include <services/bytecode_service.hpp>

namespace JuiceAgent {
namespace Services {

inline bool initializeAll() {
    auto& manager = ServiceManager::getInstance();
    manager.registerService<BytecodeService>();
    return manager.initializeAll();
}

}
}
