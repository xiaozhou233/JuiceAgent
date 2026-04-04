#pragma once

#include <eventpp/eventdispatcher.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>

class EventBus {
public:
    template<typename Event>
    using Listener = std::function<void(const Event&)>;

    template<typename Event>
    void subscribe(Listener<Event> listener, int priority = 0) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& dispatcher = getDispatcher<Event>();
        dispatcher.appendListener(priority, std::move(listener));
    }

    template<typename Event>
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        dispatchers_.erase(std::type_index(typeid(Event)));
    }

    template<typename Event>
    void post(const Event& event) {
        std::shared_ptr<void> dispatcherPtr;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = dispatchers_.find(std::type_index(typeid(Event)));
            if(it == dispatchers_.end()) return;
            dispatcherPtr = it->second;
        }

        auto dispatcher = std::static_pointer_cast<
            eventpp::EventDispatcher<int, void(const Event&)>
        >(dispatcherPtr);

        dispatcher->dispatch(0, event);
    }

private:
    std::mutex mutex_;

    std::unordered_map<std::type_index, std::shared_ptr<void>> dispatchers_;

    template<typename Event>
    eventpp::EventDispatcher<int, void(const Event&)>& getDispatcher() {
        auto typeIdx = std::type_index(typeid(Event));
        auto it = dispatchers_.find(typeIdx);

        if(it == dispatchers_.end()) {
            auto dispatcher = std::make_shared<
                eventpp::EventDispatcher<int, void(const Event&)>
            >();

            dispatchers_[typeIdx] = dispatcher;
            return *dispatcher;
        }

        return *std::static_pointer_cast<
            eventpp::EventDispatcher<int, void(const Event&)>
        >(it->second);
    }
};