#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace evt {

class ThreadSafeScopedConnection {
   public:
    ThreadSafeScopedConnection() = default;

    explicit ThreadSafeScopedConnection(std::function<void()> disconnect_fn) : m_disconnect_fn(std::move(disconnect_fn)), m_active(true) {}

    ThreadSafeScopedConnection(const ThreadSafeScopedConnection&) = delete;
    ThreadSafeScopedConnection& operator=(const ThreadSafeScopedConnection&) = delete;

    // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer) : must lock other.m_mutex before accessing its members
    ThreadSafeScopedConnection(ThreadSafeScopedConnection&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_disconnect_fn = std::move(other.m_disconnect_fn);
        m_active = other.m_active;
        other.m_active = false;
    }
    // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

    ThreadSafeScopedConnection& operator=(ThreadSafeScopedConnection&& other) noexcept {
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
            } catch (...) {  // NOLINT(bugprone-empty-catch)
                // old connection's disconnect failed; best-effort under noexcept guarantee.
            }
        }

        return *this;
    }

    ~ThreadSafeScopedConnection() noexcept { disconnect_noexcept(); }

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
class ThreadSafeDelegate;

template <typename R, typename... Args>
class ThreadSafeDelegate<R(Args...)> {
   public:
    ThreadSafeDelegate() = default;

    explicit ThreadSafeDelegate(std::function<R(Args...)> func) : m_fn(std::move(func)) {}

    template <typename F>
    static ThreadSafeDelegate from(F&& func) {
        return ThreadSafeDelegate(std::function<R(Args...)>(std::forward<F>(func)));
    }

    static ThreadSafeDelegate from(R (*func)(Args...)) { return ThreadSafeDelegate(std::function<R(Args...)>(func)); }

    template <typename T>
    static ThreadSafeDelegate from(T* obj, R (T::*method)(Args...)) {
        return ThreadSafeDelegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    template <typename T>
    static ThreadSafeDelegate from(const T* obj, R (T::*method)(Args...) const) {
        return ThreadSafeDelegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    R operator()(Args... args) const {
        assert(m_fn && "ThreadSafeDelegate::operator() called on empty delegate");
        return m_fn(std::forward<Args>(args)...);
    }

    bool valid() const { return static_cast<bool>(m_fn); }

   private:
    std::function<R(Args...)> m_fn;
};

template <typename... Args>
class ThreadSafeEvent {
   public:
    using Handler = ThreadSafeDelegate<void(Args...)>;
    using Id = std::size_t;

    ThreadSafeEvent() : m_state(std::make_shared<State>()) {}

    ThreadSafeEvent(const ThreadSafeEvent&) = delete;
    ThreadSafeEvent& operator=(const ThreadSafeEvent&) = delete;
    ThreadSafeEvent(ThreadSafeEvent&&) = delete;
    ThreadSafeEvent& operator=(ThreadSafeEvent&&) = delete;

    ThreadSafeScopedConnection subscribe(Handler handler) {
        const auto state = m_state;
        Id handler_id = 0;
        {
            std::lock_guard<std::mutex> lock(state->m_mutex);
            handler_id = state->m_next_id++;
            state->m_handlers.push_back(Slot{handler_id, std::move(handler)});
        }

        std::weak_ptr<State> weak_state = state;
        return ThreadSafeScopedConnection([weak_state, handler_id]() {
            const auto shared_state = weak_state.lock();
            if (!shared_state) {
                return;
            }
            std::lock_guard<std::mutex> lock(shared_state->m_mutex);
            auto& items = shared_state->m_handlers;
            items.erase(std::remove_if(items.begin(), items.end(), [handler_id](const Slot& slot) { return slot.id == handler_id; }), items.end());
        });
    }

    template <typename F>
    ThreadSafeScopedConnection subscribe(F&& func) {
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

    /// 触发事件: 通过 static_cast<Args> 传递参数, 支持所有 Args 类型.
    /// 值类型 Args: 每个 handler 收到独立拷贝, 多订阅者安全.
    /// 引用类型 Args: handler 共享引用.
    /// 右值引用 Args(如 ThreadSafeEvent<T&&>): 等同 std::move, 单订阅者安全,
    ///   多订阅者时后续 handler 可能收到被移动后的对象.
    void emit(Args... args) const {
        HandlerList snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            snapshot = m_state->m_handlers;
        }

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
    /// 对于右值引用 Args(如 ThreadSafeEvent<T&&>), flush 时行为与 emit() 一致:
    ///   单订阅者安全, 多订阅者时后续 handler 可能收到被移动后的对象.
    template <typename... UArgs>
    void post(UArgs&&... uargs) {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_pending.emplace_back(std::decay_t<Args>(std::forward<UArgs>(uargs))...);
    }

    /// 派发所有挂起的事件, 行为与 emit() 一致.
    /// flush 过程中新 post 的事件不会在本次 flush 中派发(防止无限循环).
    void flush() {
        std::vector<StoredArgs> batch;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            batch = std::move(m_state->m_pending);
        }
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
    std::size_t pending_count() const {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        return m_state->m_pending.size();
    }

    /// 清空挂起的事件(不触发).
    void clear_pending() {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_pending.clear();
    }

   private:
    struct Slot {
        Id id;
        Handler handler;
    };

    using HandlerList = std::vector<Slot>;
    using StoredArgs = std::tuple<std::decay_t<Args>...>;

    struct State {
        mutable std::mutex m_mutex;
        HandlerList m_handlers;
        Id m_next_id = 1;
        std::vector<StoredArgs> m_pending;
    };

    template <std::size_t... I>
    void flush_one(StoredArgs& stored, std::index_sequence<I...>) const {
        HandlerList snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            snapshot = m_state->m_handlers;
        }
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

    std::shared_ptr<State> m_state;
};

class ThreadSafeMessageBus {
   public:
    ThreadSafeMessageBus() = default;

    ThreadSafeMessageBus(const ThreadSafeMessageBus&) = delete;
    ThreadSafeMessageBus& operator=(const ThreadSafeMessageBus&) = delete;
    ThreadSafeMessageBus(ThreadSafeMessageBus&&) = delete;
    ThreadSafeMessageBus& operator=(ThreadSafeMessageBus&&) = delete;

    template <typename Message>
    ThreadSafeScopedConnection subscribe(std::function<void(const Message&)> func) {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = ensure_channel<Message>();
        return channel->m_event.subscribe(std::move(func));
    }

    template <typename Message, typename F>
    ThreadSafeScopedConnection subscribe(F&& func) {
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
    /// 先快照频道指针再逐个 flush, 避免持锁期间回调导致死锁.
    void flush() {
        std::vector<IChannel*> channel_ptrs;
        {
            std::lock_guard<std::mutex> lock(m_channels_mutex);
            channel_ptrs.reserve(m_channels.size());
            for (auto& pair : m_channels) {
                channel_ptrs.push_back(pair.second.get());
            }
        }
        std::exception_ptr first_exception;
        for (auto* channel : channel_ptrs) {
            try {
                channel->flush();
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
        std::lock_guard<std::mutex> lock(m_channels_mutex);
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
        ThreadSafeEvent<const Message&> m_event;
        void flush() override { m_event.flush(); }
        std::size_t pending_count() const override { return m_event.pending_count(); }
    };

    template <typename Message>
    TypedChannel<Message>* ensure_channel() {
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
