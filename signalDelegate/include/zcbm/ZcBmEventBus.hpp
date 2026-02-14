#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace evt {

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
            } catch (...) {  // NOLINT(bugprone-empty-catch)
                // disconnect() already set m_active = false before calling fn.
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
        } catch (...) {  // NOLINT(bugprone-empty-catch)
            // disconnect() already set m_active = false before calling fn.
        }
    }

    void disconnect() {
        if (m_active && m_disconnect_fn) {
            auto fn = std::move(m_disconnect_fn);
            m_active = false;
            fn();
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

    ZcBmEvent(const ZcBmEvent&) = delete;
    ZcBmEvent& operator=(const ZcBmEvent&) = delete;

    ZcBmEvent(ZcBmEvent&& other) noexcept : m_handlers(std::move(other.m_handlers)), m_next_id(other.m_next_id), m_pending(std::move(other.m_pending)) { other.m_next_id = 1; }

    ZcBmEvent& operator=(ZcBmEvent&& other) noexcept {
        if (this != &other) {
            m_handlers = std::move(other.m_handlers);
            m_next_id = other.m_next_id;
            m_pending = std::move(other.m_pending);
            other.m_next_id = 1;
        }
        return *this;
    }

    ZcBmScopedConnection subscribe(Handler handler) {
        ensure_handlers();
        const Id id = m_next_id++;
        m_handlers->push_back(Slot{id, std::move(handler)});

        std::weak_ptr<HandlerList> weak_handlers = m_handlers;
        return ZcBmScopedConnection([weak_handlers, id]() {
            const auto shared_handlers = weak_handlers.lock();
            if (!shared_handlers) {
                return;
            }
            auto& items = *shared_handlers;
            items.erase(std::remove_if(items.begin(), items.end(), [id](const Slot& slot) { return slot.id == id; }), items.end());
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

    /// 触发事件: 通过 static_cast<Args> 传递参数, 支持所有 Args 类型.
    /// 值类型 Args: 每个 handler 收到独立拷贝, 多订阅者安全.
    /// 引用类型 Args: handler 共享引用.
    /// 右值引用 Args(如 ZcBmEvent<T&&>): 等同 std::move, 单订阅者安全,
    ///   多订阅者时后续 handler 可能收到被移动后的对象.
    void emit(Args... args) const {
        if (!m_handlers) {
            return;
        }
        auto snapshot = *m_handlers;
        std::exception_ptr first_exception;
        for (const auto& slot : snapshot) {
            if (slot.handler.valid()) {
                try {
                    slot.handler(static_cast<Args>(args)...);
                } catch (...) {
                    if (!first_exception) {
                        first_exception = std::current_exception();
                    }
                }
            }
        }
        if (first_exception) {
            std::rethrow_exception(first_exception);
        }
    }

    void operator()(Args... args) const { emit(std::forward<Args>(args)...); }

    /// 延迟触发: 将参数 decay 后存入队列, 调用 flush() 时统一派发.
    /// 参数会被拷贝/移动到内部存储, 安全地延长生命周期.
    /// 对于右值引用 Args(如 ZcBmEvent<T&&>), flush 时行为与 emit() 一致:
    ///   单订阅者安全, 多订阅者时后续 handler 可能收到被移动后的对象.
    template <typename... UArgs>
    void post(UArgs&&... uargs) {
        m_pending.emplace_back(std::decay_t<Args>(std::forward<UArgs>(uargs))...);
    }

    /// 派发所有挂起的事件, 行为与 emit() 一致.
    /// flush 过程中新 post 的事件不会在本次 flush 中派发(防止无限循环).
    void flush() {
        auto batch = std::move(m_pending);
        std::exception_ptr first_exception;
        for (auto& args_tuple : batch) {
            try {
                flush_one(args_tuple, std::index_sequence_for<Args...>{});
            } catch (...) {
                if (!first_exception) {
                    first_exception = std::current_exception();
                }
            }
        }
        if (first_exception) {
            std::rethrow_exception(first_exception);
        }
    }

    /// 挂起事件数量.
    std::size_t pending_count() const { return m_pending.size(); }

    /// 清空挂起的事件(不触发).
    void clear_pending() { m_pending.clear(); }

   private:
    struct Slot {
        Id id;
        Handler handler;
    };

    using HandlerList = std::vector<Slot>;
    using StoredArgs = std::tuple<std::decay_t<Args>...>;

    void ensure_handlers() {
        if (!m_handlers) {
            m_handlers = std::make_shared<HandlerList>();
        }
    }

    template <std::size_t... I>
    void flush_one(StoredArgs& stored, std::index_sequence<I...>) const {
        if (!m_handlers) {
            return;
        }
        auto snapshot = *m_handlers;
        std::exception_ptr first_exception;
        for (const auto& slot : snapshot) {
            if (slot.handler.valid()) {
                try {
                    slot.handler(static_cast<Args>(std::get<I>(stored))...);
                } catch (...) {
                    if (!first_exception) {
                        first_exception = std::current_exception();
                    }
                }
            }
        }
        if (first_exception) {
            std::rethrow_exception(first_exception);
        }
    }

    std::shared_ptr<HandlerList> m_handlers;
    Id m_next_id = 1;
    std::vector<StoredArgs> m_pending;
};

class ZcBmMessageBus {
   public:
    ZcBmMessageBus() = default;

    ZcBmMessageBus(const ZcBmMessageBus&) = delete;
    ZcBmMessageBus& operator=(const ZcBmMessageBus&) = delete;

    ZcBmMessageBus(ZcBmMessageBus&&) noexcept = default;
    ZcBmMessageBus& operator=(ZcBmMessageBus&&) noexcept = default;

    template <typename Message>
    ZcBmScopedConnection subscribe(std::function<void(const Message&)> func) {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = ensure_channel<Message>();
        return channel->m_event.subscribe(std::move(func));
    }

    template <typename Message, typename F>
    ZcBmScopedConnection subscribe(F&& func) {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        return subscribe<Message>(std::function<void(const Message&)>(std::forward<F>(func)));
    }

    template <typename Message>
    void emit(const Message& message) const {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = find_channel<Message>();
        if (!channel) {
            return;
        }
        channel->m_event.emit(message);
    }

    template <typename Message>
    void clear() {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = find_channel<Message>();
        if (!channel) {
            return;
        }
        channel->m_event.clear();
    }

    /// 延迟发布: 将消息 decay 后存入队列, 调用 flush() 时统一派发.
    template <typename Message>
    void post(Message&& message) {
        using DecayedMessage = std::decay_t<Message>;
        auto* channel = ensure_channel<DecayedMessage>();
        channel->m_event.post(std::forward<Message>(message));
    }

    /// 派发所有频道中挂起的消息.
    void flush() {
        std::exception_ptr first_exception;
        for (auto& pair : m_channels) {
            try {
                pair.second->flush();
            } catch (...) {
                if (!first_exception) {
                    first_exception = std::current_exception();
                }
            }
        }
        if (first_exception) {
            std::rethrow_exception(first_exception);
        }
    }

    /// 所有频道中挂起的消息总数.
    std::size_t pending_count() const {
        std::size_t total = 0;
        for (const auto& pair : m_channels) {
            total += pair.second->pending_count();
        }
        return total;
    }

   private:
    struct IChannel {
        IChannel() = default;
        virtual ~IChannel() = default;
        IChannel(const IChannel&) = default;
        IChannel& operator=(const IChannel&) = default;
        IChannel(IChannel&&) = default;
        IChannel& operator=(IChannel&&) = default;

        virtual void flush() = 0;
        virtual std::size_t pending_count() const = 0;
    };

    template <typename Message>
    struct TypedChannel : IChannel {
        ZcBmEvent<const Message&> m_event;
        void flush() override { m_event.flush(); }
        std::size_t pending_count() const override { return m_event.pending_count(); }
    };

    template <typename Message>
    TypedChannel<Message>* ensure_channel() {
        const auto key = std::type_index(typeid(Message));
        auto it = m_channels.find(key);
        if (it == m_channels.end()) {
            auto inserted = m_channels.emplace(key, std::unique_ptr<IChannel>(new TypedChannel<Message>()));
            it = inserted.first;
        }

        auto* typed = dynamic_cast<TypedChannel<Message>*>(it->second.get());
        if (!typed) {
            throw std::logic_error("Message channel type mismatch");
        }
        return typed;
    }

    template <typename Message>
    TypedChannel<Message>* find_channel() const {
        const auto it = m_channels.find(std::type_index(typeid(Message)));
        if (it == m_channels.end()) {
            return nullptr;
        }
        auto* typed = dynamic_cast<TypedChannel<Message>*>(it->second.get());
        assert(typed && "Message channel type mismatch");
        return typed;
    }

    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> m_channels;
};

}  // namespace evt
