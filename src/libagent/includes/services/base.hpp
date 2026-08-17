#pragma once

#include <vector>
#include <utility>
#include <type_traits>
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
    void listen(EventId id, void (ServiceT::*handler)(const EventT&)) {
        static_assert(std::is_base_of<IService, ServiceT>::value,
                      "ServiceT must derive from IService");
        auto* self = static_cast<ServiceT*>(this);
        handles_.push_back({
            id,
            EventBus::getInstance().appendListener(
                id,
                [self, handler](const void* data) {
                    if (data) {
                        (self->*handler)(*static_cast<const EventT*>(data));
                    }
                }
            )
        });
    }

    // Remove all listeners registered by this service. Call it in onShutdown()
    // so no callback outlives the service instance.
    void unlistenAll() {
        auto& bus = EventBus::getInstance();
        for (auto& [id, handle] : handles_) {
            bus.removeListener(id, handle);
        }
        handles_.clear();
    }

private:
    std::vector<std::pair<EventId, EventBus::Dispatcher::Handle>> handles_;
};

}
}
