#pragma once

#include <cstddef>
#include <functional>
#include <mutex>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

// A small type-safe event bus.
//
// Listeners subscribe to a concrete event type and are invoked with that type,
// so there is no `void*` casting at the API boundary:
//
//     auto handle = EventBus::getInstance().subscribe<MyEvent>(
//         [](const MyEvent& e) { ... });
//     EventBus::getInstance().post(myEvent);
//
// Listeners are copied before dispatch, so a handler may safely subscribe or
// unsubscribe (including itself) without invalidating the iteration.
class EventBus {
public:
    using Handle = std::size_t;

    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    template <typename EventT>
    void post(const EventT& event) {
        std::vector<std::function<void(const void*)>> callbacks;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = listeners_.find(std::type_index(typeid(EventT)));
            if (it == listeners_.end()) {
                return;
            }
            callbacks.reserve(it->second.size());
            for (const auto& listener : it->second) {
                callbacks.push_back(listener.callback);
            }
        }

        for (auto& callback : callbacks) {
            callback(&event);
        }
    }

    template <typename EventT>
    Handle subscribe(std::function<void(const EventT&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);

        const Handle handle = next_handle_++;
        listeners_[std::type_index(typeid(EventT))].push_back({
            handle,
            [callback](const void* data) {
                callback(*static_cast<const EventT*>(data));
            }
        });
        return handle;
    }

    // Like subscribe(), but inserts the listener at the front so it runs
    // before previously registered listeners for the same event.
    template <typename EventT>
    Handle subscribeFront(std::function<void(const EventT&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);

        const Handle handle = next_handle_++;
        auto& list = listeners_[std::type_index(typeid(EventT))];
        list.insert(list.begin(), Listener{
            handle,
            [callback](const void* data) {
                callback(*static_cast<const EventT*>(data));
            }
        });
        return handle;
    }

    template <typename EventT>
    bool unsubscribe(Handle handle) {
        return unsubscribe(std::type_index(typeid(EventT)), handle);
    }

    // Type-erased overload, for owners that track listeners across event types.
    bool unsubscribe(std::type_index type, Handle handle) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = listeners_.find(type);
        if (it == listeners_.end()) {
            return false;
        }

        auto& list = it->second;
        for (auto e = list.begin(); e != list.end(); ++e) {
            if (e->handle == handle) {
                list.erase(e);
                return true;
            }
        }
        return false;
    }

private:
    EventBus() = default;

    struct Listener {
        Handle handle;
        std::function<void(const void*)> callback;
    };

    std::mutex mutex_;
    std::unordered_map<std::type_index, std::vector<Listener>> listeners_;
    Handle next_handle_ = 1;
};
