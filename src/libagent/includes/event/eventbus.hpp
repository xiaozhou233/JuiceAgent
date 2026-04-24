#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cstdint>

class EventBus {
public:
    using Token = std::uint64_t;

    template<typename Event>
    using Listener = std::function<void(const Event&)>;

    template<typename Event>
    Token subscribe(Listener<Event> listener, int priority = 0) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& vec = getListeners<Event>();

        Token token = nextToken_++;

        vec.push_back({
            token,
            priority,
            std::move(listener)
        });

        std::sort(vec.begin(), vec.end(),
            [](const auto& a, const auto& b) {
                return a.priority > b.priority;
            });

        return token;
    }

    template<typename Event>
    bool unsubscribe(Token token) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto type = std::type_index(typeid(Event));
        auto it = listeners_.find(type);
        if (it == listeners_.end()) {
            return false;
        }

        auto& vec =
            *std::static_pointer_cast<std::vector<Node<Event>>>(it->second);

        auto oldSize = vec.size();

        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [token](const Node<Event>& node) {
                    return node.token == token;
                }),
            vec.end()
        );

        return vec.size() != oldSize;
    }

    template<typename Event>
    void post(const Event& event) {
        std::vector<Node<Event>> copy;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto type = std::type_index(typeid(Event));
            auto it = listeners_.find(type);

            if (it == listeners_.end()) {
                return;
            }

            copy =
                *std::static_pointer_cast<std::vector<Node<Event>>>(it->second);
        }

        for (auto& node : copy) {
            node.callback(event);
        }
    }

    template<typename Event>
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.erase(std::type_index(typeid(Event)));
    }

    void clearAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.clear();
    }

private:
    template<typename Event>
    struct Node {
        Token token;
        int priority;
        Listener<Event> callback;
    };

    std::mutex mutex_;
    std::atomic<Token> nextToken_ {1};

    std::unordered_map<std::type_index, std::shared_ptr<void>> listeners_;

    template<typename Event>
    std::vector<Node<Event>>& getListeners() {
        auto type = std::type_index(typeid(Event));
        auto it = listeners_.find(type);

        if (it == listeners_.end()) {
            auto ptr = std::make_shared<std::vector<Node<Event>>>();
            listeners_[type] = ptr;
            return *ptr;
        }

        return *std::static_pointer_cast<std::vector<Node<Event>>>(it->second);
    }
};