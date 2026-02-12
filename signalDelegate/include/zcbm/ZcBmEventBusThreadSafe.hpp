#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace evt {

namespace detail_ts {

template <typename... Ts>
struct has_rvalue_ref : std::false_type {};

template <typename T, typename... Rest>
struct has_rvalue_ref<T, Rest...> : std::integral_constant<bool, std::is_rvalue_reference<T>::value || has_rvalue_ref<Rest...>::value> {};

}  // namespace detail_ts

class ZcBmTsScopedConnection {
   public:
    ZcBmTsScopedConnection() = default;

    explicit ZcBmTsScopedConnection(std::function<void()> disconnect_fn) : m_disconnect_fn(std::move(disconnect_fn)), m_active(true) {}

    ZcBmTsScopedConnection(const ZcBmTsScopedConnection&) = delete;
    ZcBmTsScopedConnection& operator=(const ZcBmTsScopedConnection&) = delete;

    // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer) : must lock other.m_mutex before accessing its members
    ZcBmTsScopedConnection(ZcBmTsScopedConnection&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_disconnect_fn = std::move(other.m_disconnect_fn);
        m_active = other.m_active;
        other.m_active = false;
    }
    // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

    ZcBmTsScopedConnection& operator=(ZcBmTsScopedConnection&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        std::function<void()> old_disconnect_fn;
        bool old_active = false;

        std::unique_lock<std::mutex> lock_self(m_mutex, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.m_mutex, std::defer_lock);
        std::lock(lock_self, lock_other);

        old_disconnect_fn = std::move(m_disconnect_fn);
        old_active = m_active;

        m_disconnect_fn = std::move(other.m_disconnect_fn);
        m_active = other.m_active;
        other.m_active = false;

        lock_self.unlock();
        lock_other.unlock();

        if (old_active && old_disconnect_fn) {
            try {
                old_disconnect_fn();
            } catch (...) {
                old_active = false;
            }
        }

        return *this;
    }

    ~ZcBmTsScopedConnection() noexcept { disconnect_noexcept(); }

    void disconnect() {
        std::function<void()> fn;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_active || !m_disconnect_fn) {
                return;
            }
            fn = std::move(m_disconnect_fn);
            m_active = false;
        }
        fn();
    }

    bool is_active() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_active;
    }

   private:
    void disconnect_noexcept() noexcept {
        try {
            disconnect();
        } catch (...) {  // NOLINT(bugprone-empty-catch)
            // m_active already set to false inside disconnect() before fn() is called.
        }
    }

    mutable std::mutex m_mutex;
    std::function<void()> m_disconnect_fn;
    bool m_active = false;
};

template <typename Signature>
class ZcBmTsDelegate;

template <typename R, typename... Args>
class ZcBmTsDelegate<R(Args...)> {
   public:
    ZcBmTsDelegate() = default;

    explicit ZcBmTsDelegate(std::function<R(Args...)> func) : m_fn(std::move(func)) {}

    template <typename F>
    static ZcBmTsDelegate from(F&& func) {
        return ZcBmTsDelegate(std::function<R(Args...)>(std::forward<F>(func)));
    }

    static ZcBmTsDelegate from(R (*func)(Args...)) { return ZcBmTsDelegate(std::function<R(Args...)>(func)); }

    template <typename T>
    static ZcBmTsDelegate from(T* obj, R (T::*method)(Args...)) {
        return ZcBmTsDelegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    template <typename T>
    static ZcBmTsDelegate from(const T* obj, R (T::*method)(Args...) const) {
        return ZcBmTsDelegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    R operator()(Args... args) const {
        assert(m_fn && "ZcBmTsDelegate::operator() called on empty delegate");
        return m_fn(std::forward<Args>(args)...);
    }

    bool valid() const { return static_cast<bool>(m_fn); }

   private:
    std::function<R(Args...)> m_fn;
};

template <typename... Args>
class ZcBmTsEvent {
   public:
    using Handler = ZcBmTsDelegate<void(Args...)>;
    using Id = std::size_t;

    ZcBmTsEvent() : m_state(std::make_shared<State>()) {}
    ~ZcBmTsEvent() = default;

    ZcBmTsEvent(const ZcBmTsEvent&) = delete;
    ZcBmTsEvent& operator=(const ZcBmTsEvent&) = delete;
    ZcBmTsEvent(ZcBmTsEvent&&) = delete;
    ZcBmTsEvent& operator=(ZcBmTsEvent&&) = delete;

    ZcBmTsScopedConnection subscribe(Handler handler) {
        const auto state = m_state;
        Id handler_id = 0;
        {
            std::lock_guard<std::mutex> lock(state->m_mutex);
            handler_id = state->m_next_id++;
            state->m_handlers.push_back(Slot{handler_id, std::move(handler)});
        }

        std::weak_ptr<State> weak_state = state;
        return ZcBmTsScopedConnection([weak_state, handler_id]() {
            const auto shared_state = weak_state.lock();
            if (!shared_state) {
                return;
            }
            std::lock_guard<std::mutex> lock(shared_state->m_mutex);
            auto& items = shared_state->m_handlers;
            items.erase(std::remove_if(items.begin(), items.end(), [handler_id](const Slot& slot) { return slot.m_id == handler_id; }), items.end());
        });
    }

    template <typename F>
    ZcBmTsScopedConnection subscribe(F&& func) {
        return subscribe(Handler::from(std::forward<F>(func)));
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_handlers.clear();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        return m_state->m_handlers.size();
    }

    void emit(Args... args) const {
        static_assert(!detail_ts::has_rvalue_ref<Args...>::value,
                      "ZcBmTsEvent::emit() does not support rvalue reference Args. "
                      "Use emit_forward() instead.");

        HandlerList snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            snapshot = m_state->m_handlers;
        }

        for (const auto& slot : snapshot) {
            if (slot.m_handler.valid()) {
                slot.m_handler(args...);
            }
        }
    }

    void emit_forward(Args... args) const {
        HandlerList snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            snapshot = m_state->m_handlers;
        }

        for (const auto& slot : snapshot) {
            if (slot.m_handler.valid()) {
                slot.m_handler(std::forward<Args>(args)...);
            }
        }
    }

    void operator()(Args... args) const { emit_forward(std::forward<Args>(args)...); }

   private:
    struct Slot {
        Id m_id;
        Handler m_handler;
    };

    using HandlerList = std::vector<Slot>;

    struct State {
        mutable std::mutex m_mutex;
        HandlerList m_handlers;
        Id m_next_id = 1;
    };

    std::shared_ptr<State> m_state;
};

class ZcBmTsMessageBus {
   public:
    ZcBmTsMessageBus() = default;
    ~ZcBmTsMessageBus() = default;

    ZcBmTsMessageBus(const ZcBmTsMessageBus&) = delete;
    ZcBmTsMessageBus& operator=(const ZcBmTsMessageBus&) = delete;
    ZcBmTsMessageBus(ZcBmTsMessageBus&&) = delete;
    ZcBmTsMessageBus& operator=(ZcBmTsMessageBus&&) = delete;

    template <typename Message>
    ZcBmTsScopedConnection subscribe(std::function<void(const Message&)> func) {
        auto* channel = find_or_create_channel<Message>();
        return channel->m_event.subscribe(std::move(func));
    }

    template <typename Message, typename F>
    ZcBmTsScopedConnection subscribe(F&& func) {
        return subscribe<Message>(std::function<void(const Message&)>(std::forward<F>(func)));
    }

    template <typename Message>
    void publish(const Message& message) const {
        auto* channel = find_channel<Message>();
        if (!channel) {
            return;
        }
        channel->m_event.emit(message);
    }

    template <typename Message>
    void clear() {
        auto* channel = find_channel<Message>();
        if (!channel) {
            return;
        }
        channel->m_event.clear();
    }

   private:
    struct IChannel {
        IChannel() = default;
        virtual ~IChannel() = default;
        IChannel(const IChannel&) = default;
        IChannel& operator=(const IChannel&) = default;
        IChannel(IChannel&&) = default;
        IChannel& operator=(IChannel&&) = default;
    };

    template <typename Message>
    struct TypedChannel : IChannel {
        ZcBmTsEvent<const Message&> m_event;
    };

    template <typename Message>
    TypedChannel<Message>* find_or_create_channel() {
        std::lock_guard<std::mutex> lock(m_channels_mutex);
        const auto key = std::type_index(typeid(Message));
        auto iter = m_channels.find(key);
        if (iter == m_channels.end()) {
            auto inserted = m_channels.emplace(key, std::unique_ptr<IChannel>(new TypedChannel<Message>()));
            iter = inserted.first;
        }

        auto* typed = dynamic_cast<TypedChannel<Message>*>(iter->second.get());
        if (!typed) {
            throw std::logic_error("Message channel type mismatch");
        }
        return typed;
    }

    template <typename Message>
    TypedChannel<Message>* find_channel() const {
        std::lock_guard<std::mutex> lock(m_channels_mutex);
        const auto iter = m_channels.find(std::type_index(typeid(Message)));
        if (iter == m_channels.end()) {
            return nullptr;
        }
        auto* typed = dynamic_cast<TypedChannel<Message>*>(iter->second.get());
        assert(typed && "Message channel type mismatch");
        return typed;
    }

    mutable std::mutex m_channels_mutex;
    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> m_channels;
};

}  // namespace evt
