#include "demo/demo.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "generic/eventBus.hpp"
#include "generic/eventBusThreadSafe.hpp"

namespace demo {

// ============================================================
// 辅助
// ============================================================
void separator(const char* title) { std::cout << "\n========== " << title << " ==========\n"; }

// ============================================================
// Demo 1: Delegate 基础 —— 自由函数、Lambda、成员函数
// ============================================================
int add(int a, int b) { return a + b; }

int Calculator::multiply(int a, int b) { return a * b; }
int Calculator::divide(int a, int b) const { return a / b; }

void demo_delegate() {
    separator("Demo 1: Delegate");
    using namespace evt;

    // 1a. 自由函数
    auto d1 = Delegate<int(int, int)>::from(&add);
    std::cout << "[自由函数]   add(3, 4)       = " << d1(3, 4) << "\n";

    // 1b. Lambda
    auto d2 = Delegate<int(int, int)>::from([](int a, int b) { return a - b; });
    std::cout << "[Lambda]     sub(10, 3)      = " << d2(10, 3) << "\n";

    // 1c. 成员函数
    Calculator calc;
    auto d3 = Delegate<int(int, int)>::from(&calc, &Calculator::multiply);
    std::cout << "[成员函数]   multiply(5, 6)  = " << d3(5, 6) << "\n";

    // 1d. const 成员函数
    const Calculator const_calc;
    auto d4 = Delegate<int(int, int)>::from(&const_calc, &Calculator::divide);
    std::cout << "[const成员]  divide(20, 4)   = " << d4(20, 4) << "\n";

    // 1e. 有返回值的 Delegate
    auto d5 = Delegate<std::string(const std::string&, const std::string&)>::from([](const std::string& a, const std::string& b) { return a + " " + b; });
    std::cout << "[返回string] concat          = " << d5("hello", "world") << "\n";

    // 1f. valid() 检查
    Delegate<void()> d_empty;
    std::cout << "[valid检查]  空Delegate      = " << (d_empty.valid() ? "true" : "false") << "\n";
    std::cout << "[valid检查]  d1              = " << (d1.valid() ? "true" : "false") << "\n";
}

// ============================================================
// Demo 2: Event 基础 —— 订阅、触发、手动取消、RAII 取消
// ============================================================
void demo_event_basic() {
    separator("Demo 2: Event 基础");
    using namespace evt;

    Event<void(int, const std::string&)> on_message;

    // 订阅三个监听器
    auto conn1 = on_message.subscribe([](int code, const std::string& msg) { std::cout << "  [监听器A] code=" << code << ", msg=" << msg << "\n"; });

    auto conn2 = on_message.subscribe([](int code, const std::string& msg) { std::cout << "  [监听器B] code=" << code << ", msg=" << msg << "\n"; });

    auto conn3 = on_message.subscribe([](int code, const std::string& msg) { std::cout << "  [监听器C] code=" << code << ", msg=" << msg << "\n"; });

    std::cout << "订阅数: " << on_message.size() << "\n";

    std::cout << "\n--- emit(200, \"OK\") -> 三个都收到 ---\n";
    on_message.emit(200, "OK");

    std::cout << "\n--- 手动 disconnect 监听器B ---\n";
    conn2.disconnect();
    std::cout << "订阅数: " << on_message.size() << "\n";
    on_message.emit(404, "Not Found");

    std::cout << "\n--- conn3 出作用域 -> RAII 自动断开 ---\n";
    {
        auto temp_conn = std::move(conn3);  // 所有权转移到局部变量
    }  // temp_conn 析构 -> 自动 disconnect
    std::cout << "订阅数: " << on_message.size() << "\n";
    on_message.emit(500, "Error");

    std::cout << "\n--- clear() 清空所有 ---\n";
    on_message.clear();
    std::cout << "订阅数: " << on_message.size() << "\n";
    on_message.emit(0, "nobody hears");
    std::cout << "(无输出 = 正确)\n";
}

// ============================================================
// Demo 3: Event + 成员函数绑定
// ============================================================
void Player::on_damage(int value) {
    hp -= value;
    std::cout << "  [" << name << "] 受到 " << value << " 伤害, hp=" << hp << "\n";
}

void Player::on_heal(int value) {
    hp += value;
    std::cout << "  [" << name << "] 恢复 " << value << " 生命, hp=" << hp << "\n";
}

void demo_event_member() {
    separator("Demo 3: Event + 成员函数");
    using namespace evt;

    Event<void(int)> damage_event;
    Event<void(int)> heal_event;

    Player warrior{"战士"};
    Player mage{"法师"};

    auto c1 = damage_event.subscribe(Delegate<void(int)>::from(&warrior, &Player::on_damage));
    auto c2 = damage_event.subscribe(Delegate<void(int)>::from(&mage, &Player::on_damage));
    auto c3 = heal_event.subscribe(Delegate<void(int)>::from(&warrior, &Player::on_heal));

    std::cout << "--- 全体受伤 20 ---\n";
    damage_event.emit(20);

    std::cout << "--- 战士恢复 10 ---\n";
    heal_event.emit(10);
}

// ============================================================
// Demo 4: Event 回调中取消订阅（快照安全）
// ============================================================
void demo_event_reentrant() {
    separator("Demo 4: 回调中取消自己(快照安全)");
    using namespace evt;

    Event<void()> on_trigger;  // 无参事件

    ScopedConnection self_conn;
    int call_count = 0;

    self_conn = on_trigger.subscribe([&]() {
        ++call_count;
        std::cout << "  一次性回调触发 (count=" << call_count << ")\n";
        self_conn.disconnect();  // 回调中取消自己
        std::cout << "  已在回调内 disconnect\n";
    });

    auto other_conn = on_trigger.subscribe([&]() { std::cout << "  普通回调触发 (count=" << call_count << ")\n"; });

    std::cout << "--- 第一次 emit ---\n";
    on_trigger.emit();

    std::cout << "--- 第二次 emit(一次性回调不再触发) ---\n";
    on_trigger.emit();
}

// ============================================================
// Demo 5: Event 嵌套 emit（回调中再触发事件）
// ============================================================
void demo_event_nested_emit() {
    separator("Demo 5: 嵌套 emit");
    using namespace evt;

    Event<void(int)> on_level;

    auto conn = on_level.subscribe([&](int level) {
        std::cout << "  收到 level=" << level << "\n";
        if (level < 3) {
            std::cout << "  -> 内部再次 emit(" << level + 1 << ")\n";
            on_level.emit(level + 1);
        }
    });

    std::cout << "--- emit(1) ---\n";
    on_level.emit(1);
}

// ============================================================
// Demo 6: MessageBus 消息总线
// ============================================================
void demo_message_bus() {
    separator("Demo 6: MessageBus");
    using namespace evt;

    MessageBus bus;

    auto c1 = bus.subscribe<DamageEvent>([](const DamageEvent& e) { std::cout << "  [伤害系统] amount=" << e.amount << ", source=" << e.source << "\n"; });

    auto c2 = bus.subscribe<DamageEvent>([](const DamageEvent& e) { std::cout << "  [日志系统] 记录伤害: " << e.amount << "\n"; });

    auto c3 = bus.subscribe<LoginEvent>([](const LoginEvent& e) { std::cout << "  [登录系统] user=" << e.user << ", time=" << e.login_time << "\n"; });

    std::cout << "--- publish DamageEvent ---\n";
    bus.emit(DamageEvent{30, "dragon"});

    std::cout << "--- publish LoginEvent ---\n";
    bus.emit(LoginEvent{"Alice", 1700000000});

    std::cout << "--- publish LogoutEvent(无人订阅, 静默跳过) ---\n";
    bus.emit(LogoutEvent{"Bob"});
    std::cout << "(无输出 = 正确)\n";

    std::cout << "\n--- 取消伤害系统订阅 ---\n";
    c1.disconnect();
    bus.emit(DamageEvent{15, "goblin"});
}

// ============================================================
// Demo 7: 地址追踪 —— 验证值传递 vs 引用传递
// ============================================================
void demo_address_by_value() {
    separator("Demo 7a: Event<void(std::string)> (按值传递)");
    using namespace evt;

    Event<void(std::string)> on_value;

    auto conn = on_value.subscribe([](std::string s) { std::cout << "  handler 内 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::string original = "hello-value";
    std::cout << "original     地址: " << (void*)original.c_str() << ", 值: \"" << original << "\"\n";

    std::cout << "--- emit(original) -> 按值, 地址应该不同 ---\n";
    on_value.emit(original);

    std::cout << "emit后 original 值: \"" << original << "\" (未被修改)\n";
}

void demo_address_by_const_ref() {
    separator("Demo 7b: Event<void(const std::string&)> (按 const 左值引用传递)");
    using namespace evt;

    Event<void(const std::string&)> on_cref;

    auto conn = on_cref.subscribe([](const std::string& s) { std::cout << "  handler 内 地址: " << (void*)&s << ", c_str: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::string original = "hello-const-ref";
    std::cout << "original     地址: " << (void*)&original << ", c_str: " << (void*)original.c_str() << ", 值: \"" << original << "\"\n";

    std::cout << "--- emit(original) -> const引用, 地址应该相同 ---\n";
    on_cref.emit(original);
}

void demo_address_by_mutable_ref() {
    separator("Demo 7c: Event<void(std::string&)> (按可变左值引用传递)");
    using namespace evt;

    Event<void(std::string&)> on_mref;

    auto conn = on_mref.subscribe([](std::string& s) {
        std::cout << "  handler 内 地址: " << (void*)&s << ", 值: \"" << s << "\"\n";
        s += " [modified]";
        std::cout << "  handler 修改后 : \"" << s << "\"\n";
    });

    std::string original = "hello-mutable-ref";
    std::cout << "original     地址: " << (void*)&original << ", 值: \"" << original << "\"\n";

    std::cout << "--- emit(original) -> 可变引用, 地址相同, 修改可见 ---\n";
    on_mref.emit(original);

    std::cout << "emit后 original 值: \"" << original << "\" (应看到修改)\n";
}

void demo_address_rvalue() {
    separator("Demo 7d: 传递右值临时对象");
    using namespace evt;

    // Event<void(std::string)> 按值接收, 可以接受右值
    Event<void(std::string)> on_value;

    auto conn = on_value.subscribe([](std::string s) { std::cout << "  handler 内 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::cout << "--- emit(临时string) -> 右值直接移动进参数 ---\n";
    on_value.emit(std::string("temporary-rvalue"));

    std::cout << "--- emit(字面量构造) ---\n";
    on_value.emit("literal-to-string");
}

void demo_address_const_ref_rvalue() {
    separator("Demo 7e: Event<void(const std::string&)> 接收右值");
    using namespace evt;

    // const 引用可以绑定到右值(临时对象)
    Event<void(const std::string&)> on_cref;

    auto conn = on_cref.subscribe([](const std::string& s) { std::cout << "  handler 内 地址: " << (void*)&s << ", 值: \"" << s << "\"\n"; });

    std::cout << "--- emit(临时string) -> const& 绑定临时对象 ---\n";
    on_cref.emit(std::string("temp-to-const-ref"));
}

// ============================================================
// Demo 7f: Delegate 单次调用支持右值引用参数
// ============================================================
void demo_delegate_rvalue_ref() {
    separator("Demo 7f: Delegate<void(std::string&&)> 右值引用参数");
    using namespace evt;

    // Delegate 是单次调用, 可以安全地接受右值引用参数
    auto d = Delegate<void(std::string&&)>::from([](std::string&& s) {
        std::cout << "  handler 内 地址: " << (void*)&s << ", c_str: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n";
        // 可以从 s 移动走
        std::string local = std::move(s);
        std::cout << "  移动后 local    : \"" << local << "\"\n";
        std::cout << "  移动后 s        : \"" << s << "\" (已被掏空)\n";
    });

    std::cout << "--- Delegate 直接接受右值 ---\n";
    d(std::string("rvalue-for-delegate"));

    std::cout << "\n--- 用 std::move 传入左值 ---\n";
    std::string named = "named-then-moved";
    std::cout << "移动前 named 地址: " << (void*)&named << ", c_str: " << (void*)named.c_str() << ", 值: \"" << named << "\"\n";
    d(std::move(named));
    std::cout << "移动后 named 值  : \"" << named << "\" (可能已被掏空)\n";
}

// ============================================================
// Demo 7g: Event<void(std::string&&)> 右值引用
// ============================================================
void demo_event_rvalue_ref() {
    separator("Demo 7g: Event<void(std::string&&)> 右值引用");
    using namespace evt;

    Event<void(std::string&&)> on_rref;

    auto conn = on_rref.subscribe([](std::string&& s) {
        std::cout << "  handler 内 地址: " << (void*)&s << ", c_str: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n";
        std::string local = std::move(s);
        std::cout << "  移动到 local   : \"" << local << "\"\n";
        std::cout << "  移动后 s       : \"" << s << "\" (已掏空)\n";
    });

    std::cout << "--- 传入临时对象 ---\n";
    on_rref.emit(std::string("rvalue-to-event"));

    std::cout << "\n--- 用 std::move 传入具名变量 ---\n";
    std::string named = "named-then-moved";
    std::cout << "移动前 named 地址: " << (void*)&named << ", c_str: " << (void*)named.c_str() << ", 值: \"" << named << "\"\n";
    on_rref.emit(std::move(named));
    std::cout << "移动后 named 值  : \"" << named << "\" (已被掏空)\n";
}

// ============================================================
// Demo 7h: Event<void(const std::string&&)> 常量右值引用
// ============================================================
void demo_event_const_rvalue_ref() {
    separator("Demo 7h: Event<void(const std::string&&)> 常量右值引用");
    using namespace evt;

    // const std::string&& : 只接受右值, 但不允许移动(const), 实践中极少使用
    Event<void(const std::string&&)> on_crref;

    auto conn = on_crref.subscribe([](const std::string&& s) {
        std::cout << "  handler 内 地址: " << (void*)&s << ", 值: \"" << s << "\"\n";
        // std::string local = std::move(s);  // ❌ 编译不过: s 是 const, 无法移动
        std::cout << "  (const 右值引用: 可以读, 不能移动 -- 实践中几乎无用)\n";
    });

    std::cout << "--- 传入临时对象 ---\n";
    on_crref.emit(std::string("const-rvalue-ref-test"));

    // std::string named = "test";
    // on_crref.emit(named);  // ❌ 左值不能绑定到 const std::string&&
}

// ============================================================
// Demo 7i: 右值引用多订阅者的注意事项
// ============================================================
void demo_emit_rvalue_caveat() {
    separator("Demo 7i: 右值引用多订阅者注意事项");
    using namespace evt;

    Event<void(std::string&&)> on_rref;

    auto c1 = on_rref.subscribe([](std::string&& s) {
        // std::string taken = std::move(s);
        std::cout << "  [订阅者1] 移走了: \"" << s << "\"\n";
    });

    auto c2 = on_rref.subscribe([](std::string&& s) {
        std::cout << "  [订阅者2] 收到  : \"" << s << "\"\n";
        std::cout << "  (第一个订阅者已经 move 走了, 所以这里是空的!)\n";
    });

    std::cout << "--- emit 多订阅者: 第一个 move 后, 后续收到空值 ---\n";
    on_rref.emit(std::string("only-first-gets-it"));

    std::cout << "\n建议: Event<T&&> 仅用于单订阅者场景 (所有权转移)\n";
    std::cout << "      多订阅者请使用 Event<const T&> 或 Event<T>\n";
}

// ============================================================
// Demo 7j: Event<void(std::string)> + std::move 正确替代方案
// ============================================================
void demo_event_value_with_move() {
    separator("Demo 7j: Event<void(std::string)> + std::move (推荐替代方案)");
    using namespace evt;

    Event<void(std::string)> on_value;

    auto conn = on_value.subscribe([](std::string s) { std::cout << "  handler 收到 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::string data = "move-into-event-value-param";
    std::cout << "移动前 data 地址: " << (void*)data.c_str() << ", 值: \"" << data << "\"\n";

    // std::move 进 emit 的按值参数, 只有一次拷贝开销(从 emit 参数到 handler 参数)
    on_value.emit(std::move(data));

    std::cout << "移动后 data 值  : \"" << data << "\" (已被移走)\n";
}

// ============================================================
// Demo 8: 多订阅者 + 值传递安全性验证
// ============================================================
void demo_multi_subscriber_value_safety() {
    separator("Demo 8: 多订阅者值传递安全性");
    using namespace evt;

    Event<void(std::string)> on_value;

    auto c1 = on_value.subscribe([](std::string s) { std::cout << "  [订阅者1] 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    auto c2 = on_value.subscribe([](std::string s) { std::cout << "  [订阅者2] 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    auto c3 = on_value.subscribe([](std::string s) { std::cout << "  [订阅者3] 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::cout << "--- 三个订阅者都应收到完整值(forward bug 修复验证) ---\n";
    on_value.emit("all-three-should-see-this");
}

// ============================================================
// Demo 9: Event 移动语义
// ============================================================
void demo_event_move() {
    separator("Demo 9: Event 移动语义");
    using namespace evt;

    Event<void(int)> e1;
    auto conn = e1.subscribe([](int x) { std::cout << "  handler: x=" << x << "\n"; });

    std::cout << "e1 订阅数: " << e1.size() << "\n";
    e1.emit(1);

    std::cout << "\n--- 移动 e1 -> e2 ---\n";
    Event<void(int)> e2 = std::move(e1);
    std::cout << "e2 订阅数: " << e2.size() << "\n";
    std::cout << "e1 订阅数: " << e1.size() << " (移动后重置为空)\n";

    std::cout << "\n--- e2.emit(2) -> handler 仍然工作 ---\n";
    e2.emit(2);

    std::cout << "--- e1.emit(3) -> 无订阅者, 无输出 ---\n";
    e1.emit(3);
    std::cout << "(无输出 = 正确)\n";

    std::cout << "\n--- disconnect 仍然有效 ---\n";
    conn.disconnect();
    std::cout << "e2 订阅数: " << e2.size() << "\n";
}

// ============================================================
// Demo 10: 复杂业务场景 —— 多服务 + MessageBus + 延迟处理(方案1)
// ============================================================

// ---------- 库存服务 ----------

InventoryService::InventoryService(evt::MessageBus& bus) : bus_(bus) {
    conns_.push_back(bus_.subscribe<OrderCreatedMsg>([this](const OrderCreatedMsg& msg) { on_order_created(msg); }));
}

void InventoryService::tick() {
    for (auto& item : pending_) {
        if (item.deducted) continue;  // 已处理，跳过

        // 模拟库存扣减逻辑
        bool ok = (item.msg.item_count <= 10);  // 假设库存只有10件
        item.deducted = true;                   // 设置标志位：已扣减

        std::cout << "  [库存服务] tick: 处理订单 #" << item.msg.order_id;
        if (ok) {
            item.success = true;
            std::cout << " -> 扣减成功 (" << item.msg.item_count << "件)\n";
        } else {
            item.success = false;
            std::cout << " -> 扣减失败 (库存不足, 需" << item.msg.item_count << "件)\n";
        }

        // 发布扣减结果，通知下游
        bus_.emit(StockDeductedMsg{
            item.msg.order_id,
            ok,
            ok ? "OK" : "库存不足",
        });
    }

    // 清理已完成的条目
    pending_.erase(std::remove_if(pending_.begin(), pending_.end(), [](const PendingDeduct& p) { return p.deducted; }), pending_.end());
}

std::size_t InventoryService::pending_count() const { return pending_.size(); }

void InventoryService::on_order_created(const OrderCreatedMsg& msg) {
    std::cout << "  [库存服务] 收到订单 #" << msg.order_id << " (用户=" << msg.user_id << ", " << msg.item_count << "件)"
              << " -> 加入待处理队列\n";
    pending_.push_back(PendingDeduct{msg, false, false});
}

// ---------- 支付服务 ----------

PaymentService::PaymentService(evt::MessageBus& bus) : bus_(bus) {
    conns_.push_back(bus_.subscribe<StockDeductedMsg>([this](const StockDeductedMsg& msg) { on_stock_deducted(msg); }));
    conns_.push_back(bus_.subscribe<OrderPaidMsg>([this](const OrderPaidMsg& msg) { on_order_paid(msg); }));
}

std::size_t PaymentService::awaiting_count() const { return awaiting_payment_.size(); }

void PaymentService::on_stock_deducted(const StockDeductedMsg& msg) {
    if (!msg.success) {
        std::cout << "  [支付服务] 订单 #" << msg.order_id << " 库存扣减失败 (" << msg.reason << "), 不进入支付流程\n";
        return;
    }
    std::cout << "  [支付服务] 订单 #" << msg.order_id << " 库存扣减成功, 等待用户支付...\n";
    awaiting_payment_.push_back(PendingPayment{msg.order_id, false});
}

void PaymentService::on_order_paid(const OrderPaidMsg& msg) {
    for (auto& item : awaiting_payment_) {
        if (item.order_id == msg.order_id && !item.confirmed) {
            item.confirmed = true;  // 标志位：已确认支付
            std::cout << "  [支付服务] 订单 #" << msg.order_id << " 支付确认 (" << msg.payment_method << ", ¥" << msg.paid_amount << ")\n";
            break;
        }
    }

    // 清理已确认的
    awaiting_payment_.erase(std::remove_if(awaiting_payment_.begin(), awaiting_payment_.end(), [](const PendingPayment& p) { return p.confirmed; }), awaiting_payment_.end());
}

// ---------- 通知服务 ----------

NotificationService::NotificationService(evt::MessageBus& bus) {
    conns_.push_back(bus.subscribe<OrderCreatedMsg>([](const OrderCreatedMsg& msg) { std::cout << "  [通知服务] -> 用户 " << msg.user_id << ": 您的订单 #" << msg.order_id << " 已创建\n"; }));

    conns_.push_back(bus.subscribe<StockDeductedMsg>([](const StockDeductedMsg& msg) {
        if (!msg.success) {
            std::cout << "  [通知服务] -> 订单 #" << msg.order_id << ": 很抱歉, " << msg.reason << ", 订单已取消\n";
        }
    }));

    conns_.push_back(bus.subscribe<OrderPaidMsg>([](const OrderPaidMsg& msg) { std::cout << "  [通知服务] -> 订单 #" << msg.order_id << ": 支付成功, 即将发货!\n"; }));
}

// ---------- 日志服务 ----------

LogService::LogService(evt::MessageBus& bus) {
    conns_.push_back(bus.subscribe<OrderCreatedMsg>(
        [](const OrderCreatedMsg& msg) { std::cout << "  [日志] ORDER_CREATED  id=" << msg.order_id << " user=" << msg.user_id << " amount=¥" << msg.total_amount << "\n"; }));

    conns_.push_back(
        bus.subscribe<StockDeductedMsg>([](const StockDeductedMsg& msg) { std::cout << "  [日志] STOCK_DEDUCTED id=" << msg.order_id << " success=" << (msg.success ? "Y" : "N") << "\n"; }));

    conns_.push_back(bus.subscribe<OrderPaidMsg>([](const OrderPaidMsg& msg) { std::cout << "  [日志] ORDER_PAID     id=" << msg.order_id << " method=" << msg.payment_method << "\n"; }));
}

// ---------- 演示函数 ----------

void demo_business_message_bus() {
    separator("Demo 10: 多服务 + MessageBus + 延迟处理(方案1)");
    using namespace evt;

    // 1. 创建总线 & 各服务
    MessageBus bus;
    InventoryService inventory(bus);
    PaymentService payment(bus);
    NotificationService notification(bus);
    LogService logger(bus);

    // 2. 模拟业务流程: 正常订单
    std::cout << "\n>>> 场景A: 正常订单 (数量=3, 库存充足)\n";
    std::cout << "--- [发布] OrderCreatedMsg ---\n";
    bus.emit(OrderCreatedMsg{1001, "Alice", 299.0, 3});

    std::cout << "\n--- [延迟] 库存服务 tick() 处理待办队列 ---\n";
    std::cout << "  (此时 pending_count=" << inventory.pending_count() << ")\n";
    inventory.tick();  // 扣减成功 → 自动发布 StockDeductedMsg → 支付服务收到

    std::cout << "\n--- [发布] OrderPaidMsg (用户完成支付) ---\n";
    bus.emit(OrderPaidMsg{1001, "微信支付", 299.0});

    // 3. 模拟业务流程: 库存不足的订单
    std::cout << "\n\n>>> 场景B: 库存不足的订单 (数量=50, 超过库存)\n";
    std::cout << "--- [发布] OrderCreatedMsg ---\n";
    bus.emit(OrderCreatedMsg{1002, "Bob", 5999.0, 50});

    std::cout << "\n--- [延迟] 库存服务 tick() ---\n";
    inventory.tick();  // 扣减失败 → StockDeductedMsg(success=false) → 支付服务忽略

    std::cout << "\n  (支付服务待支付队列长度=" << payment.awaiting_count() << ", Bob的订单未进入支付流程 = 正确)\n";

    // 4. 模拟多个订单批量到达, 延迟统一处理
    std::cout << "\n\n>>> 场景C: 批量订单 → 先收集 → 统一 tick 处理\n";
    bus.emit(OrderCreatedMsg{2001, "Charlie", 99.0, 1});
    bus.emit(OrderCreatedMsg{2002, "Dave", 199.0, 2});
    bus.emit(OrderCreatedMsg{2003, "Eve", 599.0, 5});
    std::cout << "  (三个订单入队, pending_count=" << inventory.pending_count() << ")\n";

    std::cout << "\n--- [延迟] 库存服务 tick() 一次性处理所有待办 ---\n";
    inventory.tick();

    // 5. 演示服务销毁 → 自动取消订阅
    std::cout << "\n\n>>> 场景D: 服务动态销毁 → RAII 自动断开订阅\n";
    {
        NotificationService temp_notifier(bus);
        std::cout << "--- temp_notifier 存在, 发布一个订单 ---\n";
        bus.emit(OrderCreatedMsg{9999, "Temp", 1.0, 1});
        std::cout << "  (通知服务收到了两次: notification + temp_notifier)\n";
    }  // temp_notifier 析构 → 其订阅全部断开
    std::cout << "--- temp_notifier 已销毁, 再发布 ---\n";
    bus.emit(OrderCreatedMsg{9998, "AfterDestroy", 1.0, 1});
    std::cout << "  (通知服务只收到一次: 只剩 notification)\n";
}

// ============================================================
// Demo 12: Event post + flush (延迟触发)
// ============================================================
void demo_event_post_flush() {
    separator("Demo 12: Event post + flush (延迟触发)");
    using namespace evt;

    Event<void(int, const std::string&)> on_message;

    auto c1 = on_message.subscribe([](int code, const std::string& msg) { std::cout << "  [监听器A] code=" << code << ", msg=" << msg << "\n"; });

    auto c2 = on_message.subscribe([](int code, const std::string& msg) { std::cout << "  [监听器B] code=" << code << ", msg=" << msg << "\n"; });

    std::cout << "--- post 三条消息 (不会立刻触发) ---\n";
    on_message.post(200, "OK");
    on_message.post(404, "Not Found");
    on_message.post(500, "Error");
    std::cout << "  pending_count = " << on_message.pending_count() << "\n";
    std::cout << "  (无 handler 输出 = 正确, 消息已入队但未派发)\n";

    std::cout << "\n--- flush() 统一派发 ---\n";
    on_message.flush();
    std::cout << "  pending_count = " << on_message.pending_count() << " (flush 后清空)\n";
}

// ============================================================
// Demo 13: Event post + flush 防递归 (对比 Demo 5)
// ============================================================
void demo_event_post_flush_no_recurse() {
    separator("Demo 13: post + flush 防递归 (对比 Demo 5)");
    using namespace evt;

    Event<void(int)> on_level;

    auto conn = on_level.subscribe([&](int level) {
        std::cout << "  收到 level=" << level << "\n";
        if (level < 3) {
            std::cout << "  -> post(" << level + 1 << ") 入队\n";
            on_level.post(level + 1);
        }
    });

    std::cout << "--- post(1) + 循环 flush: 广度优先, 不递归 ---\n";
    on_level.post(1);

    int round = 1;
    while (on_level.pending_count() > 0) {
        std::cout << "  [第 " << round++ << " 轮 flush, pending=" << on_level.pending_count() << "]\n";
        on_level.flush();
    }
    std::cout << "  所有事件处理完毕, pending=" << on_level.pending_count() << "\n";
}

// ============================================================
// Demo 14: MessageBus post + flush
// ============================================================
void demo_bus_post_flush() {
    separator("Demo 14: MessageBus post + flush");
    using namespace evt;

    MessageBus bus;

    auto c1 = bus.subscribe<DamageEvent>([](const DamageEvent& e) { std::cout << "  [伤害系统] amount=" << e.amount << ", source=" << e.source << "\n"; });

    auto c2 = bus.subscribe<LoginEvent>([](const LoginEvent& e) { std::cout << "  [登录系统] user=" << e.user << "\n"; });

    std::cout << "--- post 多条不同类型消息 ---\n";
    bus.post(DamageEvent{30, "dragon"});
    bus.post(DamageEvent{15, "goblin"});
    bus.post(LoginEvent{"Alice", 1700000000});
    std::cout << "  pending_count = " << bus.pending_count() << "\n";

    std::cout << "\n--- flush() 统一派发所有类型 ---\n";
    bus.flush();
    std::cout << "  pending_count = " << bus.pending_count() << "\n";

    std::cout << "\n--- 支持 move 语义: post(std::move(msg)) ---\n";
    DamageEvent big_hit{999, "final-boss"};
    std::cout << "  移动前 source: \"" << big_hit.source << "\"\n";
    bus.post(std::move(big_hit));
    std::cout << "  移动后 source: \"" << big_hit.source << "\" (已被移走)\n";
    bus.flush();
}

// ============================================================
// Demo 15: Event<void(std::string&&)> post + flush (右值引用支持)
// ============================================================
void demo_event_rvalue_post_flush() {
    separator("Demo 15: Event<void(std::string&&)> post + flush (右值引用)");
    using namespace evt;

    Event<void(std::string&&)> on_rref;

    // 单订阅者: 完全安全, 可以 move
    auto conn = on_rref.subscribe([](std::string&& s) {
        std::string taken = std::move(s);
        std::cout << "  [handler] 移走了: \"" << taken << "\"\n";
    });

    std::cout << "--- post 右值到队列 ---\n";
    on_rref.post(std::string("deferred-rvalue-1"));
    on_rref.post(std::string("deferred-rvalue-2"));
    std::cout << "  pending_count = " << on_rref.pending_count() << "\n";

    std::cout << "\n--- flush: handler 通过 std::move 拿到数据 ---\n";
    on_rref.flush();
    std::cout << "  pending_count = " << on_rref.pending_count() << "\n";

    std::cout << "\n--- 也支持从具名变量 move 进 post ---\n";
    std::string named = "named-then-posted";
    std::cout << "  移动前: \"" << named << "\"\n";
    on_rref.post(std::move(named));
    std::cout << "  移动后: \"" << named << "\" (已掏空)\n";
    on_rref.flush();
}

// ============================================================
// Demo 16: Event 返回值 + Combiner
// ============================================================
void demo_event_return_value() {
    separator("Demo 16: Event 返回值 + Combiner");
    using namespace evt;

    // 16a. CollectAll (默认): 收集所有 handler 返回值
    std::cout << "--- 16a: Event<int(int, int)> + CollectAll (默认) ---\n";
    Event<int(int, int)> compute;

    auto c1 = compute.subscribe([](int a, int b) -> int {
        std::cout << "  [handler1] " << a << " + " << b << " = " << (a + b) << "\n";
        return a + b;
    });
    auto c2 = compute.subscribe([](int a, int b) -> int {
        std::cout << "  [handler2] " << a << " * " << b << " = " << (a * b) << "\n";
        return a * b;
    });
    auto c3 = compute.subscribe([](int a, int b) -> int {
        std::cout << "  [handler3] " << a << " - " << b << " = " << (a - b) << "\n";
        return a - b;
    });

    auto results = compute.emit(10, 3);
    std::cout << "  CollectAll 结果: [";
    for (std::size_t i = 0; i < results.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << results[i];
    }
    std::cout << "]\n";

    // 16b. LastValue: 返回最后一个 handler 的值
    std::cout << "\n--- 16b: Event<int(int, int), LastValue> ---\n";
    Event<int(int, int), LastValue> last_compute;

    auto lc1 = last_compute.subscribe([](int a, int b) -> int { return a + b; });
    auto lc2 = last_compute.subscribe([](int a, int b) -> int { return a * b; });
    auto lc3 = last_compute.subscribe([](int a, int b) -> int { return a - b; });

    int last_result = last_compute.emit(10, 3);
    std::cout << "  LastValue 结果: " << last_result << " (最后一个 handler: 10 - 3 = 7)\n";

    // 16c. operator() 也返回值
    std::cout << "\n--- 16c: operator() 也返回 ResultType ---\n";
    auto op_results = compute(5, 2);
    std::cout << "  operator() 结果: [";
    for (std::size_t i = 0; i < op_results.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << op_results[i];
    }
    std::cout << "]\n";

    // 16d. post + flush 收集返回值
    std::cout << "\n--- 16d: post + flush 收集返回值 ---\n";
    compute.post(1, 2);
    compute.post(3, 4);
    auto flush_results = compute.flush();
    std::cout << "  flush 返回 " << flush_results.size() << " 个批次:\n";
    for (std::size_t i = 0; i < flush_results.size(); ++i) {
        std::cout << "    批次" << (i + 1) << ": [";
        for (std::size_t j = 0; j < flush_results[i].size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << flush_results[i][j];
        }
        std::cout << "]\n";
    }

    // 16e. MessageBus 返回值
    std::cout << "\n--- 16e: MessageBus 非 void 返回 ---\n";
    MessageBus bus;

    auto bc1 = bus.subscribe<DamageEvent, int>([](const DamageEvent& e) -> int {
        std::cout << "  [伤害计算1] base=" << e.amount << " -> " << e.amount * 2 << "\n";
        return e.amount * 2;
    });
    auto bc2 = bus.subscribe<DamageEvent, int>([](const DamageEvent& e) -> int {
        std::cout << "  [伤害计算2] base=" << e.amount << " -> " << e.amount + 10 << "\n";
        return e.amount + 10;
    });

    auto bus_results = bus.emit<DamageEvent, int>(DamageEvent{30, "dragon"});
    std::cout << "  MessageBus 结果: [";
    for (std::size_t i = 0; i < bus_results.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << bus_results[i];
    }
    std::cout << "]\n";
}

// ============================================================
// 运行所有 demo
// ============================================================
void run_all_demos() {
    demo_delegate();
    demo_event_basic();
    demo_event_member();
    demo_event_reentrant();
    demo_event_nested_emit();
    demo_message_bus();

    // 地址追踪 & 引用语义验证
    demo_address_by_value();
    demo_address_by_const_ref();
    demo_address_by_mutable_ref();
    demo_address_rvalue();
    demo_address_const_ref_rvalue();
    demo_delegate_rvalue_ref();
    demo_event_rvalue_ref();
    demo_event_const_rvalue_ref();
    demo_emit_rvalue_caveat();
    demo_event_value_with_move();
    demo_multi_subscriber_value_safety();

    demo_event_move();

    // 复杂业务场景
    demo_business_message_bus();

    // post + flush 延迟触发
    demo_event_post_flush();
    demo_event_post_flush_no_recurse();
    demo_bus_post_flush();
    demo_event_rvalue_post_flush();

    // Event 返回值 + Combiner
    demo_event_return_value();

    std::cout << "\n========== All demos done ==========\n";
}

// ============================================================
// 线程安全 demo
// ============================================================
namespace {

struct ParallelMsg {
    int producer;
    int seq;
};

void demo_parallel_threadsafe() {
    separator("Demo 11: 并行 ThreadSafeEvent/MessageBus");

    evt::ThreadSafeEvent<void(const ParallelMsg&)> event;
    std::atomic<int> handled{0};

    auto h1 = event.subscribe([&](const ParallelMsg& m) {
        ++handled;
        if (m.seq % 10 == 0) {
            std::cout << "[E1] producer=" << m.producer << ", seq=" << m.seq << "\n";
        }
    });
    auto h2 = event.subscribe([&](const ParallelMsg& m) {
        ++handled;
        if (m.seq % 15 == 0) {
            std::cout << "[E2] producer=" << m.producer << ", seq=" << m.seq << "\n";
        }
    });

    std::vector<std::thread> workers;
    workers.reserve(4);
    for (int p = 0; p < 4; ++p) {
        workers.emplace_back([p, &event]() {
            for (int i = 1; i <= 30; ++i) {
                event.emit(ParallelMsg{p, i});
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    for (auto& t : workers) {
        t.join();
    }

    std::cout << "ThreadSafeEvent handled count=" << handled.load() << "\n";
    h1.disconnect();
    h2.disconnect();

    evt::ThreadSafeMessageBus bus;
    std::atomic<int> login_count{0};
    auto c = bus.subscribe<LoginEvent>([&](const LoginEvent& e) {
        ++login_count;
        std::cout << "[BUS] login " << e.user << "\n";
    });

    std::thread t1([&]() { bus.emit(LoginEvent{"U1", 1}); });
    std::thread t2([&]() { bus.emit(LoginEvent{"U2", 2}); });
    std::thread t3([&]() { bus.emit(LoginEvent{"U3", 3}); });
    t1.join();
    t2.join();
    t3.join();

    c.disconnect();
    std::cout << "ThreadSafeMessageBus login_count=" << login_count.load() << "\n";
}

}  // namespace

void run_parallel_demo() { demo_parallel_threadsafe(); }

}  // namespace demo
