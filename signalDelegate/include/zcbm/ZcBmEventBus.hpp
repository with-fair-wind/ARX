#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace evt {

namespace detail {

template <typename... Ts>
struct has_rvalue_ref : std::false_type {};

template <typename T, typename... Rest>
struct has_rvalue_ref<T, Rest...> : std::integral_constant<bool, std::is_rvalue_reference<T>::value || has_rvalue_ref<Rest...>::value> {};

}  // namespace detail

class ZcBmScopedConnection {
   public:
    ZcBmScopedConnection() = default;

    explicit ZcBmScopedConnection(std::function<void()> disconnect_fn) : m_disconnect_fn(std::move(disconnect_fn)), m_active(true) {}

    ZcBmScopedConnection(const ZcBmScopedConnection&) = delete;
    ZcBmScopedConnection& operator=(const ZcBmScopedConnection&) = delete;

    ZcBmScopedConnection(ZcBmScopedConnection&& other) noexcept : m_disconnect_fn(std::move(other.m_disconnect_fn)), m_active(other.m_active) { other.m_active = false; }

    ZcBmScopedConnection& operator=(ZcBmScopedConnection&& other) noexcept {
        if (this != &other) {
            try {
                disconnect();
            } catch (...) {
                // Keep noexcept guarantee for move assignment.
                m_active = false;
            }
            m_disconnect_fn = std::move(other.m_disconnect_fn);
            m_active = other.m_active;
            other.m_active = false;
        }
        return *this;
    }

    ~ZcBmScopedConnection() noexcept {
        try {
            disconnect();
        } catch (...) {
            // Best-effort cleanup in destructor: exceptions must not escape.
            m_active = false;
        }
    }

    void disconnect() {
        if (m_active && m_disconnect_fn) {
            m_disconnect_fn();
            m_active = false;
        }
    }

    bool is_active() const { return m_active; }

   private:
    std::function<void()> m_disconnect_fn;
    bool m_active = false;
};

template <typename Signature>
class ZcBmDelegate;

template <typename R, typename... Args>
class ZcBmDelegate<R(Args...)> {
   public:
    ZcBmDelegate() = default;

    explicit ZcBmDelegate(std::function<R(Args...)> func) : m_fn(std::move(func)) {}

    template <typename F>
    static ZcBmDelegate from(F&& func) {
        return ZcBmDelegate(std::function<R(Args...)>(std::forward<F>(func)));
    }

    static ZcBmDelegate from(R (*func)(Args...)) { return ZcBmDelegate(std::function<R(Args...)>(func)); }

    template <typename T>
    static ZcBmDelegate from(T* obj, R (T::*method)(Args...)) {
        return ZcBmDelegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    template <typename T>
    static ZcBmDelegate from(const T* obj, R (T::*method)(Args...) const) {
        return ZcBmDelegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    R operator()(Args... args) const {
        assert(m_fn && "ZcBmDelegate::operator() called on empty delegate");
        return m_fn(std::forward<Args>(args)...);
    }

    bool valid() const { return static_cast<bool>(m_fn); }

   private:
    std::function<R(Args...)> m_fn;
};

template <typename... Args>
class ZcBmEvent {
   public:
    using Handler = ZcBmDelegate<void(Args...)>;
    using Id = std::size_t;

    ZcBmEvent() : m_handlers(std::make_shared<HandlerList>()) {}

    ~ZcBmEvent() = default;

    ZcBmEvent(const ZcBmEvent&) = delete;
    ZcBmEvent& operator=(const ZcBmEvent&) = delete;

    ZcBmEvent(ZcBmEvent&& other) noexcept : m_handlers(std::move(other.m_handlers)), m_next_id(other.m_next_id) { other.m_next_id = 1; }

    ZcBmEvent& operator=(ZcBmEvent&& other) noexcept {
        if (this != &other) {
            m_handlers = std::move(other.m_handlers);
            m_next_id = other.m_next_id;
            other.m_next_id = 1;
        }
        return *this;
    }

    ZcBmScopedConnection subscribe(Handler handler) {
        ensure_handlers();
        const Id handler_id = m_next_id++;
        m_handlers->push_back(Slot{handler_id, std::move(handler)});

        std::weak_ptr<HandlerList> weak_handlers = m_handlers;
        return ZcBmScopedConnection([weak_handlers, handler_id]() {
            const auto shared_handlers = weak_handlers.lock();
            if (!shared_handlers) {
                return;
            }
            auto& items = *shared_handlers;
            items.erase(std::remove_if(items.begin(), items.end(), [handler_id](const Slot& slot) { return slot.m_id == handler_id; }), items.end());
        });
    }

    template <typename F>
    ZcBmScopedConnection subscribe(F&& func) {
        return subscribe(Handler::from(std::forward<F>(func)));
    }

    void clear() {
        if (m_handlers) {
            m_handlers->clear();
        }
    }

    std::size_t size() const { return m_handlers ? m_handlers->size() : 0; }

    /// 安全触发: 每个 handler 都以左值接收参数, 多订阅者安全.
    /// 不支持右值引用 Args (编译期报错), 请改用 emit_forward().
    void emit(Args... args) const {
        static_assert(!detail::has_rvalue_ref<Args...>::value,
                      "ZcBmEvent::emit() does not support rvalue reference Args. "
                      "Use emit_forward() instead.");
        if (!m_handlers) {
            return;
        }
        auto snapshot = *m_handlers;
        for (const auto& slot : snapshot) {
            if (slot.m_handler.valid()) {
                slot.m_handler(args...);
            }
        }
    }

    /// 转发触发: 使用 std::forward 传递参数, 支持右值引用 Args.
    /// 注意: 对于值类型/右值引用, 第一个 handler 可能移走数据,
    ///       后续 handler 收到的是被移动后的对象. 单订阅者时完全安全.
    void emit_forward(Args... args) const {
        if (!m_handlers) {
            return;
        }
        auto snapshot = *m_handlers;
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

    void ensure_handlers() {
        if (!m_handlers) {
            m_handlers = std::make_shared<HandlerList>();
        }
    }

    std::shared_ptr<HandlerList> m_handlers;
    Id m_next_id = 1;
};

class ZcBmMessageBus {
   public:
    ZcBmMessageBus() = default;

    ~ZcBmMessageBus() = default;

    ZcBmMessageBus(const ZcBmMessageBus&) = delete;
    ZcBmMessageBus& operator=(const ZcBmMessageBus&) = delete;

    ZcBmMessageBus(ZcBmMessageBus&&) noexcept = default;
    ZcBmMessageBus& operator=(ZcBmMessageBus&&) noexcept = default;

    template <typename Message>
    ZcBmScopedConnection subscribe(std::function<void(const Message&)> func) {
        auto& channel = channel_for<Message>();
        return channel.m_event.subscribe(std::move(func));
    }

    template <typename Message, typename F>
    ZcBmScopedConnection subscribe(F&& func) {
        return subscribe<Message>(std::function<void(const Message&)>(std::forward<F>(func)));
    }

    template <typename Message>
    void publish(const Message& message) const {
        const auto iter = m_channels.find(std::type_index(typeid(Message)));
        if (iter == m_channels.end()) {
            return;
        }
        auto* channel = dynamic_cast<TypedChannel<Message>*>(iter->second.get());
        assert(channel && "Message channel type mismatch");
        channel->m_event.emit(message);
    }

    template <typename Message>
    void clear() {
        auto iter = m_channels.find(std::type_index(typeid(Message)));
        if (iter == m_channels.end()) {
            return;
        }
        auto* channel = dynamic_cast<TypedChannel<Message>*>(iter->second.get());
        assert(channel && "Message channel type mismatch");
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
        ZcBmEvent<const Message&> m_event;
    };

    template <typename Message>
    TypedChannel<Message>& channel_for() {
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
        return *typed;
    }

    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> m_channels;
};

}  // namespace evt
