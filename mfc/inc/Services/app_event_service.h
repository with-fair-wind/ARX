#pragma once

#include <event_system.hpp>

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

// ============================================================
// 消息类型定义
// 约定: 带脏标记管理的事件需提供 bool dirty 字段和 reset() 方法
// ============================================================

/// 族模板变更事件
struct TemplateChangedEvent {
    bool dirty = false;
    std::wstring template_name;

    void markDirty(const std::wstring& name) {
        dirty = true;
        template_name = name;
    }

    void reset() {
        dirty = false;
        template_name.clear();
    }
};

// 新增事件只需定义 struct, 提供 dirty + reset() 即可:
// struct XxxEvent {
//     bool dirty = false;
//     void markDirty(...) { dirty = true; ... }
//     void reset() { dirty = false; ... }
// };

// ============================================================
// 应用级事件服务 (单例)
//
// 流程:
//   业务层:  AppEventService::instance().event<TemplateChangedEvent>().markDirty(L"xxx");
//   Reactor: AppEventService::instance().flush();
//   UI 层:   conn = AppEventService::instance().bus()
//                .subscribe<TemplateChangedEvent>([this](auto& e) { refresh(); });
// ============================================================
class AppEventService {
   public:
    static AppEventService& instance() {
        static AppEventService s;
        return s;
    }

    /// 消息总线 (UI 层订阅 / 也可用于即时 publish)
    evt::MessageBus& bus() { return bus_; }

    /// 获取/自动注册一个带脏标记的事件 (首次调用时创建)
    template <typename T>
    T& event() {
        const auto key = std::type_index(typeid(T));
        auto it = holders_.find(key);
        if (it == holders_.end()) {
            auto* holder = new EventHolder<T>();
            holders_.emplace(key, std::unique_ptr<IEventHolder>(holder));
            return holder->data;
        }
        return static_cast<EventHolder<T>*>(it->second.get())->data;
    }

    /// Reactor 在 commandEnded 时调用: 检查所有脏事件, 通过 bus 发布
    void flush() {
        for (auto& pair : holders_) {
            pair.second->flush(bus_);
        }
    }

    void reset() {
        for (auto& pair : holders_) {
            pair.second->reset();
        }
    }

    AppEventService(const AppEventService&) = delete;
    AppEventService& operator=(const AppEventService&) = delete;

   private:
    AppEventService() = default;

    struct IEventHolder {
        virtual ~IEventHolder() = default;
        virtual void flush(evt::MessageBus& bus) = 0;
        virtual void reset() = 0;
    };

    /// EventHolder: 检查 dirty, 通过 bus 发布, 然后 reset
    template <typename T>
    struct EventHolder : IEventHolder {
        T data;

        void flush(evt::MessageBus& bus) override {
            if (data.dirty) {
                bus.publish(data);
                data.reset();
            }
        }

        void reset() override { data.reset(); }
    };

    evt::MessageBus bus_;
    std::unordered_map<std::type_index, std::unique_ptr<IEventHolder>> holders_;
};
