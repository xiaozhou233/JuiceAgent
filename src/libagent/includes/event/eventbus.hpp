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

    // === Readonly ===
    template<typename Event>
    using Listener = std::function<void(const Event&)>;

    // === Mutable ===
    template<typename Event>
    using MutableListener = std::function<void(Event&)>;

    // =========================
    // Subscribe (Readonly)
    // =========================
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

        sort(vec);
        return token;
    }

    // =========================
    // Subscribe (Mutable)
    // =========================
    template<typename Event>
    Token subscribe_mutable(MutableListener<Event> listener, int priority = 0) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto& vec = getMutableListeners<Event>();

        Token token = nextToken_++;

        vec.push_back({
            token,
            priority,
            std::move(listener)
        });

        sort(vec);
        return token;
    }

    // =========================
    // Unsubscribe
    // =========================
    template<typename Event>
    bool unsubscribe(Token token) {
        std::lock_guard<std::mutex> lock(mutex_);

        bool removed = false;

        // readonly
        if (auto it = listeners_.find(std::type_index(typeid(Event))); it != listeners_.end()) {
            auto& vec = *std::static_pointer_cast<std::vector<Node<Event>>>(it->second);
            removed |= erase(vec, token);
        }

        // mutable
        if (auto it = mutable_listeners_.find(std::type_index(typeid(Event))); it != mutable_listeners_.end()) {
            auto& vec = *std::static_pointer_cast<std::vector<MutableNode<Event>>>(it->second);
            removed |= erase(vec, token);
        }

        return removed;
    }

    // =========================
    // Post (Readonly)
    // =========================
    template<typename Event>
    void post(const Event& event) {
        std::vector<Node<Event>> copy;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = listeners_.find(std::type_index(typeid(Event)));
            if (it == listeners_.end()) return;

            copy = *std::static_pointer_cast<std::vector<Node<Event>>>(it->second);
        }

        for (auto& node : copy) {
            node.callback(event);
        }
    }

    // =========================
    // Post (Mutable)
    // =========================
    template<typename Event>
    void post_mutable(Event& event) {
        std::vector<MutableNode<Event>> copy;

        {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = mutable_listeners_.find(std::type_index(typeid(Event)));
            if (it == mutable_listeners_.end()) return;

            copy = *std::static_pointer_cast<std::vector<MutableNode<Event>>>(it->second);
        }

        for (auto& node : copy) {
            node.callback(event);
        }
    }

    // =========================
    // Clear
    // =========================
    template<typename Event>
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.erase(std::type_index(typeid(Event)));
        mutable_listeners_.erase(std::type_index(typeid(Event)));
    }

    void clearAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.clear();
        mutable_listeners_.clear();
    }

private:
    template<typename Event>
    struct Node {
        Token token;
        int priority;
        Listener<Event> callback;
    };

    template<typename Event>
    struct MutableNode {
        Token token;
        int priority;
        MutableListener<Event> callback;
    };

    std::mutex mutex_;
    std::atomic<Token> nextToken_{1};

    std::unordered_map<std::type_index, std::shared_ptr<void>> listeners_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> mutable_listeners_;

    // === helpers ===

    template<typename T>
    static void sort(std::vector<T>& vec) {
        std::sort(vec.begin(), vec.end(),
            [](const auto& a, const auto& b) {
                return a.priority > b.priority;
            });
    }

    template<typename T>
    static bool erase(std::vector<T>& vec, Token token) {
        auto oldSize = vec.size();

        vec.erase(
            std::remove_if(vec.begin(), vec.end(),
                [token](const T& node) {
                    return node.token == token;
                }),
            vec.end()
        );

        return vec.size() != oldSize;
    }

    template<typename Event>
    std::vector<Node<Event>>& getListeners() {
        return getContainer<Event, Node<Event>>(listeners_);
    }

    template<typename Event>
    std::vector<MutableNode<Event>>& getMutableListeners() {
        return getContainer<Event, MutableNode<Event>>(mutable_listeners_);
    }

    template<typename Event, typename NodeType>
    std::vector<NodeType>& getContainer(std::unordered_map<std::type_index, std::shared_ptr<void>>& map) {
        auto type = std::type_index(typeid(Event));
        auto it = map.find(type);

        if (it == map.end()) {
            auto ptr = std::make_shared<std::vector<NodeType>>();
            map[type] = ptr;
            return *ptr;
        }

        return *std::static_pointer_cast<std::vector<NodeType>>(it->second);
    }
};