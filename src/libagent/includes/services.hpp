#pragma once

#include <services/base.hpp>
#include <services/service_manager.hpp>

namespace JuiceAgent {
namespace Services {

inline bool initializeAll() {
    auto& manager = ServiceManager::getInstance();
    ServiceRegistry::getInstance().instantiateAll(manager);
    return manager.initializeAll();
}

}
}
