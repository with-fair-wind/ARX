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
            } catch (...) {  // NOLINT(bugprone-empty-catch)
                // old connection's disconnect failed; best-effort under noexcept guarantee.
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

// ---- Combiners ----

/// 默认合并策略: 收集所有 handler 返回值到 std::vector.
template <typename R>
struct ZcBmTsCollectAll {
    using result_type = std::vector<R>;
    static result_type combine(std::vector<R>&& results) { return std::move(results); }
};

/// 返回最后一个 handler 的值. 要求至少一个 handler, 否则 assert 失败.
template <typename R>
struct ZcBmTsLastValue {
    using result_type = R;
    static result_type combine(std::vector<R>&& results) {  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved) : we move an element, not the container
        assert(!results.empty() && "ZcBmTsLastValue combiner requires at least one handler");
        return std::move(results.back());
    }
};

// ---- ZcBmTsEvent ----

template <typename Signature, template <typename> class CombinerT = ZcBmTsCollectAll>
class ZcBmTsEvent;

/// ZcBmTsEvent 特化: void 返回值 — 行为与传统 ZcBmTsEvent 完全一致, CombinerT 被忽略.
template <typename... Args, template <typename> class CombinerT>
class ZcBmTsEvent<void(Args...), CombinerT> {
   public:
    using Handler = ZcBmTsDelegate<void(Args...)>;
    using Id = std::size_t;

    ZcBmTsEvent() : m_state(std::make_shared<State>()) {}

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
            items.erase(std::remove_if(items.begin(), items.end(), [handler_id](const Slot& slot) { return slot.id == handler_id; }), items.end());
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

    /// 触发事件: 通过 static_cast<Args> 传递参数, 支持所有 Args 类型.
    /// 值类型 Args: 每个 handler 收到独立拷贝, 多订阅者安全.
    /// 引用类型 Args: handler 共享引用.
    /// 右值引用 Args(如 ZcBmTsEvent<void(T&&)>): 等同 std::move, 单订阅者安全,
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
    /// 对于右值引用 Args(如 ZcBmTsEvent<void(T&&)>), flush 时行为与 emit() 一致:
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

/// ZcBmTsEvent 特化: 非 void 返回值 — handler 返回值通过 CombinerT 合并.
/// emit() 返回 Combiner::result_type, flush() 返回 std::vector<result_type>.
template <typename R, typename... Args, template <typename> class CombinerT>
class ZcBmTsEvent<R(Args...), CombinerT> {
   public:
    using Handler = ZcBmTsDelegate<R(Args...)>;
    using Id = std::size_t;
    using Combiner = CombinerT<R>;
    using ResultType = typename Combiner::result_type;

    ZcBmTsEvent() : m_state(std::make_shared<State>()) {}

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
            items.erase(std::remove_if(items.begin(), items.end(), [handler_id](const Slot& slot) { return slot.id == handler_id; }), items.end());
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

    /// 触发事件: 调用所有 handler, 收集返回值, 通过 Combiner 合并.
    /// 值类型 Args: 每个 handler 收到独立拷贝, 多订阅者安全.
    /// 引用类型 Args: handler 共享引用.
    /// 右值引用 Args(如 ZcBmTsEvent<int(T&&)>): 等同 std::move, 单订阅者安全,
    ///   多订阅者时后续 handler 可能收到被移动后的对象.
    ResultType emit(Args... args) const {
        HandlerList snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            snapshot = m_state->m_handlers;
        }

        if (snapshot.empty()) {
            return Combiner::combine(std::vector<R>{});
        }

        std::vector<R> results;
        std::exception_ptr first_exception;
        for (const auto& slot : snapshot) {
            if (slot.handler.valid()) {
                try {
                    results.push_back(slot.handler(static_cast<Args>(args)...));
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
        return Combiner::combine(std::move(results));
    }

    ResultType operator()(Args... args) const { return emit(std::forward<Args>(args)...); }

    /// 延迟触发: 将参数 decay 后存入队列, 调用 flush() 时统一派发.
    /// 参数会被拷贝/移动到内部存储, 安全地延长生命周期.
    template <typename... UArgs>
    void post(UArgs&&... uargs) {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_pending.emplace_back(std::decay_t<Args>(std::forward<UArgs>(uargs))...);
    }

    /// 派发所有挂起的事件, 每个批次产生一个合并结果.
    /// flush 过程中新 post 的事件不会在本次 flush 中派发(防止无限循环).
    std::vector<ResultType> flush() {
        std::vector<StoredArgs> batch;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            batch = std::move(m_state->m_pending);
        }
        std::vector<ResultType> all_results;
        std::exception_ptr first_exception;
        for (auto& args_tuple : batch) {
            try {
                all_results.push_back(flush_one(args_tuple, std::index_sequence_for<Args...>{}));
            } catch (...) {
                if (!first_exception) {
                    first_exception = std::current_exception();
                }
            }
        }
        if (first_exception) {
            std::rethrow_exception(first_exception);
        }
        return all_results;
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
    ResultType flush_one(StoredArgs& stored, std::index_sequence<I...>) const {
        HandlerList snapshot;
        {
            std::lock_guard<std::mutex> lock(m_state->m_mutex);
            snapshot = m_state->m_handlers;
        }

        if (snapshot.empty()) {
            return Combiner::combine(std::vector<R>{});
        }

        std::vector<R> results;
        std::exception_ptr first_exception;
        for (const auto& slot : snapshot) {
            if (slot.handler.valid()) {
                try {
                    results.push_back(slot.handler(static_cast<Args>(std::get<I>(stored))...));
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
        return Combiner::combine(std::move(results));
    }

    std::shared_ptr<State> m_state;
};

// ---- ZcBmTsMessageBus ----

class ZcBmTsMessageBus {
   public:
    ZcBmTsMessageBus() = default;

    ZcBmTsMessageBus(const ZcBmTsMessageBus&) = delete;
    ZcBmTsMessageBus& operator=(const ZcBmTsMessageBus&) = delete;
    ZcBmTsMessageBus(ZcBmTsMessageBus&&) = delete;
    ZcBmTsMessageBus& operator=(ZcBmTsMessageBus&&) = delete;

    // ---- void 返回 (向后兼容) ----

    template <typename Message>
    ZcBmTsScopedConnection subscribe(std::function<void(const Message&)> func) {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = ensure_channel<Message>();
        return channel->m_event.subscribe(std::move(func));
    }

    template <typename Message, typename F>
    ZcBmTsScopedConnection subscribe(F&& func) {
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

    // ---- 非 void 返回 (R 必须显式指定) ----

    template <typename Message, typename R, template <typename> class CombinerTC = ZcBmTsCollectAll,
              typename std::enable_if<!std::is_void<R>::value, int>::type = 0>
    ZcBmTsScopedConnection subscribe(std::function<R(const Message&)> func) {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = ensure_ret_channel<Message, R, CombinerTC>();
        return channel->m_event.subscribe(std::move(func));
    }

    template <typename Message, typename R, template <typename> class CombinerTC = ZcBmTsCollectAll,
              typename std::enable_if<!std::is_void<R>::value, int>::type = 0>
    typename CombinerTC<R>::result_type emit(const Message& message) const {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = find_ret_channel<Message, R, CombinerTC>();
        if (!channel) {
            return typename CombinerTC<R>::result_type{};
        }
        return channel->m_event.emit(message);
    }

    template <typename Message, typename R, template <typename> class CombinerTC = ZcBmTsCollectAll,
              typename std::enable_if<!std::is_void<R>::value, int>::type = 0>
    void clear() {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = find_ret_channel<Message, R, CombinerTC>();
        if (!channel) {
            return;
        }
        channel->m_event.clear();
    }

    // ---- post / flush ----

    /// 延迟发布 (void 频道).
    template <typename Message>
    void post(Message&& message) {
        using DecayedMessage = std::decay_t<Message>;
        auto* channel = ensure_channel<DecayedMessage>();
        channel->m_event.post(std::forward<Message>(message));
    }

    /// 延迟发布 (非 void 频道, R 必须显式指定).
    template <typename Message, typename R, template <typename> class CombinerTC = ZcBmTsCollectAll,
              typename std::enable_if<!std::is_void<R>::value, int>::type = 0>
    void post(const Message& message) {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = ensure_ret_channel<Message, R, CombinerTC>();
        channel->m_event.post(message);
    }

    /// 派发所有频道中挂起的消息 (非 void 频道的返回值被丢弃).
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

    /// 按频道 flush 并返回结果 (非 void 频道).
    template <typename Message, typename R, template <typename> class CombinerTC = ZcBmTsCollectAll,
              typename std::enable_if<!std::is_void<R>::value, int>::type = 0>
    std::vector<typename CombinerTC<R>::result_type> flush() {
        static_assert(std::is_same<Message, std::decay_t<Message>>::value, "Message type must not be cv-qualified or a reference. Use the base type.");
        auto* channel = find_ret_channel<Message, R, CombinerTC>();
        if (!channel) {
            return {};
        }
        return channel->m_event.flush();
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
        ZcBmTsEvent<void(const Message&)> m_event;
        void flush() override { m_event.flush(); }
        std::size_t pending_count() const override { return m_event.pending_count(); }
    };

    template <typename Message, typename R, template <typename> class CombinerTC>
    struct TypedRetChannel : IChannel {
        ZcBmTsEvent<R(const Message&), CombinerTC> m_event;
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

    template <typename Message, typename R, template <typename> class CombinerTC>
    TypedRetChannel<Message, R, CombinerTC>* ensure_ret_channel() {
        std::lock_guard<std::mutex> lock(m_channels_mutex);
        const auto key = std::type_index(typeid(Message));
        auto iter = m_channels.find(key);
        if (iter == m_channels.end()) {
            auto inserted = m_channels.emplace(key, std::unique_ptr<IChannel>(new TypedRetChannel<Message, R, CombinerTC>()));
            iter = inserted.first;
        }

        auto* typed = dynamic_cast<TypedRetChannel<Message, R, CombinerTC>*>(iter->second.get());
        if (!typed) {
            throw std::logic_error("Message channel type/return-type mismatch");
        }
        return typed;
    }

    template <typename Message, typename R, template <typename> class CombinerTC>
    TypedRetChannel<Message, R, CombinerTC>* find_ret_channel() const {
        std::lock_guard<std::mutex> lock(m_channels_mutex);
        const auto iter = m_channels.find(std::type_index(typeid(Message)));
        if (iter == m_channels.end()) {
            return nullptr;
        }
        auto* typed = dynamic_cast<TypedRetChannel<Message, R, CombinerTC>*>(iter->second.get());
        assert(typed && "Message channel type/return-type mismatch");
        return typed;
    }

    mutable std::mutex m_channels_mutex;
    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> m_channels;
};

}  // namespace evt
