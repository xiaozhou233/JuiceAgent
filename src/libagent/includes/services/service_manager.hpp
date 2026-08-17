#pragma once

#include <services/base.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
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

    IService* createService(std::unique_ptr<IService> service) {
        IService* raw = service.get();
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

// Static self-registration: each service's .cpp uses JUICEAGENT_REGISTER_SERVICE.
class ServiceRegistry {
public:
    using Factory = std::function<std::unique_ptr<IService>()>;

    static ServiceRegistry& getInstance() {
        static ServiceRegistry instance;
        return instance;
    }

    void addFactory(Factory factory) {
        std::lock_guard<std::mutex> lock(mutex_);
        factories_.push_back(std::move(factory));
    }

    void instantiateAll(ServiceManager& manager) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& f : factories_) {
            manager.createService(f());
        }
    }

private:
    ServiceRegistry() = default;
    std::mutex mutex_;
    std::vector<Factory> factories_;
};

template <typename ServiceT>
class ServiceRegistrar {
public:
    ServiceRegistrar() {
        ServiceRegistry::getInstance().addFactory(
            []() -> std::unique_ptr<IService> {
                return std::make_unique<ServiceT>();
            }
        );
    }
};

}
}

#define JUICEAGENT_SERVICE_CONCAT_IMPL(a, b) a##b
#define JUICEAGENT_SERVICE_CONCAT(a, b) JUICEAGENT_SERVICE_CONCAT_IMPL(a, b)

// Put at file scope in the service's .cpp:
//   JUICEAGENT_REGISTER_SERVICE(juiceagent::services::MyService)
#define JUICEAGENT_REGISTER_SERVICE(ServiceT)                                     \
    namespace {                                                                  \
        const ::JuiceAgent::Services::ServiceRegistrar<ServiceT>                 \
            JUICEAGENT_SERVICE_CONCAT(_juiceagent_service_registrar_, __LINE__); \
    }
