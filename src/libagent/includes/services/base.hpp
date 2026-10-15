#pragma once

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
    template <typename ServiceT, typename EventT>
    void listen(void (ServiceT::*handler)(const EventT&)) {
        static_assert(std::is_base_of<IService, ServiceT>::value,
                      "ServiceT must derive from IService");
        auto* self = static_cast<ServiceT*>(this);
        const EventBus::Handle handle =
            EventBus::getInstance().subscribe<EventT>(
                [self, handler](const EventT& event) {
                    (self->*handler)(event);
                });
        handles_.push_back({std::type_index(typeid(EventT)), handle});
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
    std::vector<std::pair<std::type_index, EventBus::Handle>> handles_;
};

}
}
