#pragma once

#include <functional>
#include <generic/eventBus.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================
// 消息类型定义
// 说明: 事件类型本身不需要实现任何固定字段或方法
// ============================================================

/// 族模板变更事件
struct TemplateChangedEvent {
    std::wstring template_name;
};

// ============================================================
// 应用级事件服务 (单例)
//
// 流程:
//   业务层:  AppEventService::instance().post(TemplateChangedEvent{L"xxx"});
//   Reactor: AppEventService::instance().flush();
//   UI 层:   AppEventService::instance().subscribeManaged<TemplateChangedEvent>(this, [this](const auto&) { refresh(); });
//   UI 析构: AppEventService::instance().unsubscribeOwner(this);
// ============================================================
class AppEventService {
   public:
    static AppEventService& instance() {
        static AppEventService s;
        return s;
    }

    /// 消息总线 (UI 层订阅 / 也可用于即时 publish)
    evt::MessageBus& bus() { return bus_; }

    /// 延迟发布一个事件（在 flush() 时统一 publish）
    template <typename T>
    void post(const T& event) {
        pending_events_.emplace_back([event](evt::MessageBus& bus) { bus.publish(event); });
    }

    /// 延迟构造并发布一个事件（在 flush() 时统一 publish）
    template <typename T, typename... Args>
    void emplace(Args&&... args) {
        post<T>(T{std::forward<Args>(args)...});
    }

    /// 由服务托管订阅连接，owner 通常传 this
    template <typename Message, typename F>
    void subscribeManaged(const void* owner, F&& callback) {
        if (owner == nullptr) {
            return;
        }
        auto connection = bus_.subscribe<Message>(std::forward<F>(callback));
        managed_connections_[owner].push_back(std::move(connection));
    }

    /// 取消 owner 对应的全部订阅
    void unsubscribeOwner(const void* owner) { managed_connections_.erase(owner); }

    /// 取消全部托管订阅
    void clearAllSubscriptions() { managed_connections_.clear(); }

    /// Reactor 在 commandEnded 时调用: 发布队列中的全部事件
    void flush() {
        for (const auto& publish_fn : pending_events_) {
            publish_fn(bus_);
        }
        pending_events_.clear();
    }

    void reset() {
        pending_events_.clear();
        managed_connections_.clear();
    }

    AppEventService(const AppEventService&) = delete;
    AppEventService& operator=(const AppEventService&) = delete;
    AppEventService(AppEventService&&) = delete;
    AppEventService& operator=(AppEventService&&) = delete;
    ~AppEventService() = default;

   private:
    AppEventService() = default;

    evt::MessageBus bus_;
    std::vector<std::function<void(evt::MessageBus&)>> pending_events_;
    std::unordered_map<const void*, std::vector<evt::ScopedConnection>> managed_connections_;
};
