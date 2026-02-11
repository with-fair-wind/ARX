#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace evt {

namespace detail {

template <typename... Ts>
struct has_rvalue_ref : std::false_type {};

template <typename T, typename... Rest>
struct has_rvalue_ref<T, Rest...>
    : std::integral_constant<bool,
                             std::is_rvalue_reference<T>::value || has_rvalue_ref<Rest...>::value> {};

}  // namespace detail

class ScopedConnection {
   public:
    ScopedConnection() = default;

    explicit ScopedConnection(std::function<void()> disconnect_fn) : disconnect_fn_(std::move(disconnect_fn)), active_(true) {}

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    ScopedConnection(ScopedConnection&& other) noexcept : disconnect_fn_(std::move(other.disconnect_fn_)), active_(other.active_) { other.active_ = false; }

    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            disconnect();
            disconnect_fn_ = std::move(other.disconnect_fn_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }

    ~ScopedConnection() { disconnect(); }

    void disconnect() {
        if (active_ && disconnect_fn_) {
            disconnect_fn_();
            active_ = false;
        }
    }

    bool is_active() const { return active_; }

   private:
    std::function<void()> disconnect_fn_;
    bool active_ = false;
};

template <typename Signature>
class Delegate;

template <typename R, typename... Args>
class Delegate<R(Args...)> {
   public:
    Delegate() = default;

    explicit Delegate(std::function<R(Args...)> fn) : fn_(std::move(fn)) {}

    template <typename F>
    static Delegate from(F&& f) {
        return Delegate(std::function<R(Args...)>(std::forward<F>(f)));
    }

    static Delegate from(R (*func)(Args...)) { return Delegate(std::function<R(Args...)>(func)); }

    template <typename T>
    static Delegate from(T* obj, R (T::*method)(Args...)) {
        return Delegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    template <typename T>
    static Delegate from(const T* obj, R (T::*method)(Args...) const) {
        return Delegate([obj, method](Args... args) -> R { return (obj->*method)(std::forward<Args>(args)...); });
    }

    R operator()(Args... args) const {
        assert(fn_ && "Delegate::operator() called on empty delegate");
        return fn_(std::forward<Args>(args)...);
    }

    bool valid() const { return static_cast<bool>(fn_); }

   private:
    std::function<R(Args...)> fn_;
};

template <typename... Args>
class Event {
   public:
    using Handler = Delegate<void(Args...)>;
    using Id = std::size_t;

    Event() : handlers_(std::make_shared<HandlerList>()) {}

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&& other) : handlers_(std::move(other.handlers_)), next_id_(other.next_id_) {
        other.handlers_ = std::make_shared<HandlerList>();
        other.next_id_ = 1;
    }

    Event& operator=(Event&& other) {
        if (this != &other) {
            handlers_ = std::move(other.handlers_);
            next_id_ = other.next_id_;
            other.handlers_ = std::make_shared<HandlerList>();
            other.next_id_ = 1;
        }
        return *this;
    }

    ScopedConnection subscribe(Handler handler) {
        const Id id = next_id_++;
        handlers_->push_back(Slot{id, std::move(handler)});

        std::weak_ptr<HandlerList> weak_handlers = handlers_;
        return ScopedConnection([weak_handlers, id]() {
            const auto shared_handlers = weak_handlers.lock();
            if (!shared_handlers) {
                return;
            }
            auto& items = *shared_handlers;
            items.erase(std::remove_if(items.begin(), items.end(), [id](const Slot& slot) { return slot.id == id; }), items.end());
        });
    }

    template <typename F>
    ScopedConnection subscribe(F&& f) {
        return subscribe(Handler::from(std::forward<F>(f)));
    }

    void clear() { handlers_->clear(); }

    std::size_t size() const { return handlers_->size(); }

    /// 安全触发: 每个 handler 都以左值接收参数, 多订阅者安全.
    /// 不支持右值引用 Args (编译期报错), 请改用 emit_forward().
    void emit(Args... args) const {
        static_assert(!detail::has_rvalue_ref<Args...>::value,
                      "Event::emit() does not support rvalue reference Args. "
                      "Use emit_forward() instead.");
        auto snapshot = *handlers_;
        for (const auto& slot : snapshot) {
            if (slot.handler.valid()) {
                slot.handler(args...);
            }
        }
    }

    /// 转发触发: 使用 std::forward 传递参数, 支持右值引用 Args.
    /// 注意: 对于值类型/右值引用, 第一个 handler 可能移走数据,
    ///       后续 handler 收到的是被移动后的对象. 单订阅者时完全安全.
    void emit_forward(Args... args) const {
        auto snapshot = *handlers_;
        for (const auto& slot : snapshot) {
            if (slot.handler.valid()) {
                slot.handler(std::forward<Args>(args)...);
            }
        }
    }

    void operator()(Args... args) const { emit_forward(std::forward<Args>(args)...); }

   private:
    struct Slot {
        Id id;
        Handler handler;
    };

    using HandlerList = std::vector<Slot>;

    std::shared_ptr<HandlerList> handlers_;
    Id next_id_ = 1;
};

class MessageBus {
   public:
    MessageBus() = default;

    MessageBus(const MessageBus&) = delete;
    MessageBus& operator=(const MessageBus&) = delete;

    MessageBus(MessageBus&&) = default;
    MessageBus& operator=(MessageBus&&) = default;

    template <typename Message>
    ScopedConnection subscribe(std::function<void(const Message&)> fn) {
        auto& channel = channel_for<Message>();
        return channel.event.subscribe(std::move(fn));
    }

    template <typename Message, typename F>
    ScopedConnection subscribe(F&& f) {
        return subscribe<Message>(std::function<void(const Message&)>(std::forward<F>(f)));
    }

    template <typename Message>
    void publish(const Message& message) const {
        const auto it = channels_.find(std::type_index(typeid(Message)));
        if (it == channels_.end()) {
            return;
        }
        auto* channel = dynamic_cast<TypedChannel<Message>*>(it->second.get());
        assert(channel && "Message channel type mismatch");
        channel->event.emit(message);
    }

    template <typename Message>
    void clear() {
        auto it = channels_.find(std::type_index(typeid(Message)));
        if (it == channels_.end()) {
            return;
        }
        auto* channel = dynamic_cast<TypedChannel<Message>*>(it->second.get());
        assert(channel && "Message channel type mismatch");
        channel->event.clear();
    }

   private:
    struct IChannel {
        virtual ~IChannel() {}
    };

    template <typename Message>
    struct TypedChannel : IChannel {
        Event<const Message&> event;
    };

    template <typename Message>
    TypedChannel<Message>& channel_for() {
        const auto key = std::type_index(typeid(Message));
        auto it = channels_.find(key);
        if (it == channels_.end()) {
            auto inserted = channels_.emplace(key, std::unique_ptr<IChannel>(new TypedChannel<Message>()));
            it = inserted.first;
        }

        auto* typed = dynamic_cast<TypedChannel<Message>*>(it->second.get());
        if (!typed) {
            throw std::logic_error("Message channel type mismatch");
        }
        return *typed;
    }

    std::unordered_map<std::type_index, std::unique_ptr<IChannel>> channels_;
};

}  // namespace evt
