#pragma once

#include <functional>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include <JuiceAgent/Logger.hpp>
#include <eventbus.hpp>

namespace JuiceAgent {
namespace Services {

class IService {
public:
    virtual ~IService() = default;

    virtual const char* name() const = 0;

    virtual bool onInitialize() { return true; }

    virtual void onShutdown() {}

protected:
    // Register `handler` to run after previously registered listeners.
    template <typename ServiceT, typename EventT>
    void listen(void (ServiceT::*handler)(const EventT&)) {
        static_assert(std::is_base_of<IService, ServiceT>::value,
                      "ServiceT must derive from IService");
        auto* self = static_cast<ServiceT*>(this);
        trackListener<EventT>(
            [self, handler](const EventT& event) { (self->*handler)(event); },
            false);
    }

    // Register `handler` to run before previously registered listeners.
    // Use it when the ordering between listeners matters (e.g. a transformer
    // pipeline that must run before other consumers).
    template <typename ServiceT, typename EventT>
    void listenFront(void (ServiceT::*handler)(const EventT&)) {
        static_assert(std::is_base_of<IService, ServiceT>::value,
                      "ServiceT must derive from IService");
        auto* self = static_cast<ServiceT*>(this);
        trackListener<EventT>(
            [self, handler](const EventT& event) { (self->*handler)(event); },
            true);
    }

    // Remove all listeners registered by this service. Call it in onShutdown()
    // so no callback outlives the service instance.
    void unlistenAll() {
        auto& bus = EventBus::getInstance();
        for (auto& [type, handle] : handles_) {
            bus.unsubscribe(type, handle);
        }
        handles_.clear();
    }

private:
    template <typename EventT>
    void trackListener(std::function<void(const EventT&)> callback, bool front) {
        auto& bus = EventBus::getInstance();
        const EventBus::Handle handle = front
            ? bus.subscribeFront<EventT>(std::move(callback))
            : bus.subscribe<EventT>(std::move(callback));
        handles_.push_back({std::type_index(typeid(EventT)), handle});
    }

    std::vector<std::pair<std::type_index, EventBus::Handle>> handles_;
};

}
}
