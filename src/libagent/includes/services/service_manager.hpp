#pragma once

#include <services/base.hpp>

#include <algorithm>
#include <memory>
#include <vector>

namespace JuiceAgent {
namespace Services {

class ServiceManager {
public:
    static ServiceManager& getInstance() {
        static ServiceManager instance;
        return instance;
    }

    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    template <typename ServiceT, typename... Args>
    ServiceT* registerService(Args&&... args) {
        auto service = std::make_unique<ServiceT>(std::forward<Args>(args)...);
        ServiceT* raw = service.get();
        spdlog::debug("[ServiceManager] Registering service: {}", raw->name());
        services_.push_back(std::move(service));
        return raw;
    }

    template <typename ServiceT>
    ServiceT* getService() const {
        const std::type_info& target = typeid(ServiceT);
        for (const auto& s : services_) {
            if (typeid(*s) == target) {
                return static_cast<ServiceT*>(s.get());
            }
        }
        return nullptr;
    }

    bool initializeAll() {
        for (auto& s : services_) {
            spdlog::info("[ServiceManager] Initializing service: {}", s->name());
            if (!s->onInitialize()) {
                spdlog::error("[ServiceManager] Service '{}' failed to initialize", s->name());
                return false;
            }
        }
        return true;
    }

    void shutdownAll() {
        for (auto it = services_.rbegin(); it != services_.rend(); ++it) {
            spdlog::info("[ServiceManager] Shutting down service: {}", (*it)->name());
            (*it)->onShutdown();
        }
        services_.clear();
    }

private:
    ServiceManager() = default;
    ~ServiceManager() {
        shutdownAll();
    }

    std::vector<std::unique_ptr<IService>> services_;
};

}
}
