#pragma once

#include <eventpp/eventdispatcher.h>
#include <event/event_type.hpp>

enum class EventId {
    ClassFileLoadHook,
    MethodEntry,
    MethodExit,
    PreLoad,
    Loaded
};

class EventBus {
public:
    using Dispatcher = eventpp::EventDispatcher<EventId, void(const void*)>;

    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    void post(EventId id, const void* event) {
        dispatcher.dispatch(id, event);
    }

    template <typename C>
    auto appendListener(EventId id, const C& callback) {
        return dispatcher.appendListener(id, callback);
    }

    template <typename C>
    auto prependListener(EventId id, const C& callback) {
        return dispatcher.prependListener(id, callback);
    }

    bool removeListener(EventId id, Dispatcher::Handle handle) {
        return dispatcher.removeListener(id, handle);
    }

private:
    EventBus() = default;
    Dispatcher dispatcher;
};