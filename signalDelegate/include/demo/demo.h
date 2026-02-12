#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "generic/eventBus.hpp"

namespace demo {

// ============================================================
// 辅助
// ============================================================
void separator(const char* title);

// ============================================================
// Demo 1: Delegate 基础 —— 自由函数、Lambda、成员函数
// ============================================================
int add(int a, int b);

struct Calculator {
    int multiply(int a, int b);
    int divide(int a, int b) const;
};

void demo_delegate();

// ============================================================
// Demo 2: Event 基础 —— 订阅、触发、手动取消、RAII 取消
// ============================================================
void demo_event_basic();

// ============================================================
// Demo 3: Event + 成员函数绑定
// ============================================================
struct Player {
    std::string name;
    int hp = 100;

    void on_damage(int value);
    void on_heal(int value);
};

void demo_event_member();

// ============================================================
// Demo 4: Event 回调中取消订阅（快照安全）
// ============================================================
void demo_event_reentrant();

// ============================================================
// Demo 5: Event 嵌套 emit（回调中再触发事件）
// ============================================================
void demo_event_nested_emit();

// ============================================================
// Demo 6: MessageBus 消息总线
// ============================================================
struct DamageEvent {
    int amount;
    std::string source;
};

struct LoginEvent {
    std::string user;
    int login_time;
};

struct LogoutEvent {
    std::string user;
};

void demo_message_bus();

// ============================================================
// Demo 7: 地址追踪 —— 验证值传递 vs 引用传递
// ============================================================
void demo_address_by_value();
void demo_address_by_const_ref();
void demo_address_by_mutable_ref();
void demo_address_rvalue();
void demo_address_const_ref_rvalue();

// ============================================================
// Demo 7f: Delegate 单次调用支持右值引用参数
// ============================================================
void demo_delegate_rvalue_ref();

// ============================================================
// Demo 7g: Event<std::string&&> 通过 emit_forward 支持右值引用
// ============================================================
void demo_event_rvalue_ref();

// ============================================================
// Demo 7h: Event<const std::string&&> 常量右值引用
// ============================================================
void demo_event_const_rvalue_ref();

// ============================================================
// Demo 7i: emit_forward 多订阅者的注意事项
// ============================================================
void demo_emit_forward_caveat();

// ============================================================
// Demo 7j: Event<std::string> + std::move 正确替代方案
// ============================================================
void demo_event_value_with_move();

// ============================================================
// Demo 8: 多订阅者 + 值传递安全性验证
// ============================================================
void demo_multi_subscriber_value_safety();

// ============================================================
// Demo 9: Event 移动语义
// ============================================================
void demo_event_move();

// ============================================================
// Demo 10: 复杂业务场景 —— 多服务 + MessageBus + 延迟处理(方案1)
// ============================================================

// ---------- 消息契约(所有模块共享的纯数据结构) ----------

/// 用户下单
struct OrderCreatedMsg {
    int64_t order_id;
    std::string user_id;
    double total_amount;
    int item_count;
};

/// 支付完成
struct OrderPaidMsg {
    int64_t order_id;
    std::string payment_method;
    double paid_amount;
};

/// 库存扣减结果
struct StockDeductedMsg {
    int64_t order_id;
    bool success;
    std::string reason;
};

/// 订单完成通知
struct OrderFulfilledMsg {
    int64_t order_id;
    std::string user_id;
};

// ---------- 库存服务 ----------

class InventoryService {
   public:
    explicit InventoryService(evt::MessageBus& bus);
    void tick();
    std::size_t pending_count() const;

   private:
    void on_order_created(const OrderCreatedMsg& msg);

    struct PendingDeduct {
        OrderCreatedMsg msg;
        bool deducted;
        bool success;
    };

    evt::MessageBus& bus_;
    std::vector<PendingDeduct> pending_;
    std::vector<evt::ScopedConnection> conns_;
};

// ---------- 支付服务 ----------

class PaymentService {
   public:
    explicit PaymentService(evt::MessageBus& bus);
    std::size_t awaiting_count() const;

   private:
    void on_stock_deducted(const StockDeductedMsg& msg);
    void on_order_paid(const OrderPaidMsg& msg);

    struct PendingPayment {
        int64_t order_id;
        bool confirmed;
    };

    evt::MessageBus& bus_;
    std::vector<PendingPayment> awaiting_payment_;
    std::vector<evt::ScopedConnection> conns_;
};

// ---------- 通知服务 ----------

class NotificationService {
   public:
    explicit NotificationService(evt::MessageBus& bus);

   private:
    std::vector<evt::ScopedConnection> conns_;
};

// ---------- 日志服务 ----------

class LogService {
   public:
    explicit LogService(evt::MessageBus& bus);

   private:
    std::vector<evt::ScopedConnection> conns_;
};

void demo_business_message_bus();

// ============================================================
// 运行所有 demo / 线程安全 demo
// ============================================================
void run_all_demos();
void run_parallel_demo();

}  // namespace demo
