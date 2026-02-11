#include <iostream>
#include <string>
#include <vector>

#include "event_system.hpp"

// ============================================================
// 辅助
// ============================================================
static void separator(const char* title) { std::cout << "\n========== " << title << " ==========\n"; }

// ============================================================
// Demo 1: Delegate 基础 —— 自由函数、Lambda、成员函数
// ============================================================
static int add(int a, int b) { return a + b; }

struct Calculator {
    int multiply(int a, int b) { return a * b; }
    int divide(int a, int b) const { return a / b; }
};

static void demo_delegate() {
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
static void demo_event_basic() {
    separator("Demo 2: Event 基础");
    using namespace evt;

    Event<int, const std::string&> on_message;

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
struct Player {
    std::string name;
    int hp = 100;

    void on_damage(int value) {
        hp -= value;
        std::cout << "  [" << name << "] 受到 " << value << " 伤害, hp=" << hp << "\n";
    }

    void on_heal(int value) {
        hp += value;
        std::cout << "  [" << name << "] 恢复 " << value << " 生命, hp=" << hp << "\n";
    }
};

static void demo_event_member() {
    separator("Demo 3: Event + 成员函数");
    using namespace evt;

    Event<int> damage_event;
    Event<int> heal_event;

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
static void demo_event_reentrant() {
    separator("Demo 4: 回调中取消自己(快照安全)");
    using namespace evt;

    Event<> on_trigger;  // 无参事件

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
static void demo_event_nested_emit() {
    separator("Demo 5: 嵌套 emit");
    using namespace evt;

    Event<int> on_level;

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

static void demo_message_bus() {
    separator("Demo 6: MessageBus");
    using namespace evt;

    MessageBus bus;

    auto c1 = bus.subscribe<DamageEvent>([](const DamageEvent& e) { std::cout << "  [伤害系统] amount=" << e.amount << ", source=" << e.source << "\n"; });

    auto c2 = bus.subscribe<DamageEvent>([](const DamageEvent& e) { std::cout << "  [日志系统] 记录伤害: " << e.amount << "\n"; });

    auto c3 = bus.subscribe<LoginEvent>([](const LoginEvent& e) { std::cout << "  [登录系统] user=" << e.user << ", time=" << e.login_time << "\n"; });

    std::cout << "--- publish DamageEvent ---\n";
    bus.publish(DamageEvent{30, "dragon"});

    std::cout << "--- publish LoginEvent ---\n";
    bus.publish(LoginEvent{"Alice", 1700000000});

    std::cout << "--- publish LogoutEvent(无人订阅, 静默跳过) ---\n";
    bus.publish(LogoutEvent{"Bob"});
    std::cout << "(无输出 = 正确)\n";

    std::cout << "\n--- 取消伤害系统订阅 ---\n";
    c1.disconnect();
    bus.publish(DamageEvent{15, "goblin"});
}

// ============================================================
// Demo 7: 地址追踪 —— 验证值传递 vs 引用传递
// ============================================================
static void demo_address_by_value() {
    separator("Demo 7a: Event<std::string> (按值传递)");
    using namespace evt;

    Event<std::string> on_value;

    auto conn = on_value.subscribe([](std::string s) { std::cout << "  handler 内 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::string original = "hello-value";
    std::cout << "original     地址: " << (void*)original.c_str() << ", 值: \"" << original << "\"\n";

    std::cout << "--- emit(original) -> 按值, 地址应该不同 ---\n";
    on_value.emit(original);

    std::cout << "emit后 original 值: \"" << original << "\" (未被修改)\n";
}

static void demo_address_by_const_ref() {
    separator("Demo 7b: Event<const std::string&> (按 const 左值引用传递)");
    using namespace evt;

    Event<const std::string&> on_cref;

    auto conn = on_cref.subscribe([](const std::string& s) { std::cout << "  handler 内 地址: " << (void*)&s << ", c_str: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::string original = "hello-const-ref";
    std::cout << "original     地址: " << (void*)&original << ", c_str: " << (void*)original.c_str() << ", 值: \"" << original << "\"\n";

    std::cout << "--- emit(original) -> const引用, 地址应该相同 ---\n";
    on_cref.emit(original);
}

static void demo_address_by_mutable_ref() {
    separator("Demo 7c: Event<std::string&> (按可变左值引用传递)");
    using namespace evt;

    Event<std::string&> on_mref;

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

static void demo_address_rvalue() {
    separator("Demo 7d: 传递右值临时对象");
    using namespace evt;

    // Event<std::string> 按值接收, 可以接受右值
    Event<std::string> on_value;

    auto conn = on_value.subscribe([](std::string s) { std::cout << "  handler 内 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::cout << "--- emit(临时string) -> 右值直接移动进参数 ---\n";
    on_value.emit(std::string("temporary-rvalue"));

    std::cout << "--- emit(字面量构造) ---\n";
    on_value.emit("literal-to-string");
}

static void demo_address_const_ref_rvalue() {
    separator("Demo 7e: Event<const std::string&> 接收右值");
    using namespace evt;

    // const 引用可以绑定到右值(临时对象)
    Event<const std::string&> on_cref;

    auto conn = on_cref.subscribe([](const std::string& s) { std::cout << "  handler 内 地址: " << (void*)&s << ", 值: \"" << s << "\"\n"; });

    std::cout << "--- emit(临时string) -> const& 绑定临时对象 ---\n";
    on_cref.emit(std::string("temp-to-const-ref"));
}

// ============================================================
// Demo 7f: Delegate 单次调用支持右值引用参数
// ============================================================
static void demo_delegate_rvalue_ref() {
    separator("Demo 7f: Delegate<void(std::string&&)> 右值引用参数");
    using namespace evt;

    // Delegate 是单次调用, 可以安全地接受右值引用参数
    auto d = Delegate<void(std::string&&)>::from([](std::string&& s) {
        std::cout << "  handler 内 地址: " << (void*)&s
                  << ", c_str: " << (void*)s.c_str()
                  << ", 值: \"" << s << "\"\n";
        // 可以从 s 移动走
        std::string local = std::move(s);
        std::cout << "  移动后 local    : \"" << local << "\"\n";
        std::cout << "  移动后 s        : \"" << s << "\" (已被掏空)\n";
    });

    std::cout << "--- Delegate 直接接受右值 ---\n";
    d(std::string("rvalue-for-delegate"));

    std::cout << "\n--- 用 std::move 传入左值 ---\n";
    std::string named = "named-then-moved";
    std::cout << "移动前 named 地址: " << (void*)&named
              << ", c_str: " << (void*)named.c_str()
              << ", 值: \"" << named << "\"\n";
    d(std::move(named));
    std::cout << "移动后 named 值  : \"" << named << "\" (可能已被掏空)\n";
}

// ============================================================
// Demo 7g: Event<std::string&&> 通过 emit_forward 支持右值引用
// ============================================================
static void demo_event_rvalue_ref() {
    separator("Demo 7g: Event<std::string&&> 右值引用 (emit_forward)");
    using namespace evt;

    // Event<std::string&&>: emit() 会触发 static_assert, 必须用 emit_forward()
    Event<std::string&&> on_rref;

    auto conn = on_rref.subscribe([](std::string&& s) {
        std::cout << "  handler 内 地址: " << (void*)&s
                  << ", c_str: " << (void*)s.c_str()
                  << ", 值: \"" << s << "\"\n";
        std::string local = std::move(s);
        std::cout << "  移动到 local   : \"" << local << "\"\n";
        std::cout << "  移动后 s       : \"" << s << "\" (已掏空)\n";
    });

    std::cout << "--- 传入临时对象 ---\n";
    on_rref.emit_forward(std::string("rvalue-to-event"));

    std::cout << "\n--- 用 std::move 传入具名变量 ---\n";
    std::string named = "named-then-moved";
    std::cout << "移动前 named 地址: " << (void*)&named
              << ", c_str: " << (void*)named.c_str()
              << ", 值: \"" << named << "\"\n";
    on_rref.emit_forward(std::move(named));
    std::cout << "移动后 named 值  : \"" << named << "\" (已被掏空)\n";

    // 注意: emit() 会编译报错 (static_assert), 取消下行注释可验证:
    // on_rref.emit(std::string("will not compile"));
}

// ============================================================
// Demo 7h: Event<const std::string&&> 常量右值引用
// ============================================================
static void demo_event_const_rvalue_ref() {
    separator("Demo 7h: Event<const std::string&&> 常量右值引用 (emit_forward)");
    using namespace evt;

    // const std::string&& : 只接受右值, 但不允许移动(const), 实践中极少使用
    Event<const std::string&&> on_crref;

    auto conn = on_crref.subscribe([](const std::string&& s) {
        std::cout << "  handler 内 地址: " << (void*)&s
                  << ", 值: \"" << s << "\"\n";
        // std::string local = std::move(s);  // ❌ 编译不过: s 是 const, 无法移动
        std::cout << "  (const 右值引用: 可以读, 不能移动 -- 实践中几乎无用)\n";
    });

    std::cout << "--- 传入临时对象 ---\n";
    on_crref.emit_forward(std::string("const-rvalue-ref-test"));

    // std::string named = "test";
    // on_crref.emit_forward(named);  // ❌ 左值不能绑定到 const std::string&&
}

// ============================================================
// Demo 7i: emit_forward 多订阅者的注意事项
// ============================================================
static void demo_emit_forward_caveat() {
    separator("Demo 7i: emit_forward 多订阅者注意事项");
    using namespace evt;

    Event<std::string&&> on_rref;

    auto c1 = on_rref.subscribe([](std::string&& s) {
        std::string taken = std::move(s);
        std::cout << "  [订阅者1] 移走了: \"" << taken << "\"\n";
    });

    auto c2 = on_rref.subscribe([](std::string&& s) {
        std::cout << "  [订阅者2] 收到  : \"" << s << "\"\n";
        std::cout << "  (第一个订阅者已经 move 走了, 所以这里是空的!)\n";
    });

    std::cout << "--- emit_forward 多订阅者: 第一个 move 后, 后续收到空值 ---\n";
    on_rref.emit_forward(std::string("only-first-gets-it"));

    std::cout << "\n建议: Event<T&&> 仅用于单订阅者场景 (所有权转移)\n";
    std::cout << "      多订阅者请使用 Event<const T&> 或 Event<T>\n";
}

// ============================================================
// Demo 7j: Event<std::string> + std::move 正确替代方案
// ============================================================
static void demo_event_value_with_move() {
    separator("Demo 7j: Event<std::string> + std::move (推荐替代方案)");
    using namespace evt;

    Event<std::string> on_value;

    auto conn = on_value.subscribe([](std::string s) {
        std::cout << "  handler 收到 地址: " << (void*)s.c_str()
                  << ", 值: \"" << s << "\"\n";
    });

    std::string data = "move-into-event-value-param";
    std::cout << "移动前 data 地址: " << (void*)data.c_str()
              << ", 值: \"" << data << "\"\n";

    // std::move 进 emit 的按值参数, 只有一次拷贝开销(从 emit 参数到 handler 参数)
    on_value.emit(std::move(data));

    std::cout << "移动后 data 值  : \"" << data << "\" (已被移走)\n";
}

// ============================================================
// Demo 8: 多订阅者 + 值传递安全性验证
// ============================================================
static void demo_multi_subscriber_value_safety() {
    separator("Demo 8: 多订阅者值传递安全性");
    using namespace evt;

    Event<std::string> on_value;

    auto c1 = on_value.subscribe([](std::string s) { std::cout << "  [订阅者1] 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    auto c2 = on_value.subscribe([](std::string s) { std::cout << "  [订阅者2] 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    auto c3 = on_value.subscribe([](std::string s) { std::cout << "  [订阅者3] 地址: " << (void*)s.c_str() << ", 值: \"" << s << "\"\n"; });

    std::cout << "--- 三个订阅者都应收到完整值(forward bug 修复验证) ---\n";
    on_value.emit("all-three-should-see-this");
}

// ============================================================
// Demo 9: Event 移动语义
// ============================================================
static void demo_event_move() {
    separator("Demo 9: Event 移动语义");
    using namespace evt;

    Event<int> e1;
    auto conn = e1.subscribe([](int x) { std::cout << "  handler: x=" << x << "\n"; });

    std::cout << "e1 订阅数: " << e1.size() << "\n";
    e1.emit(1);

    std::cout << "\n--- 移动 e1 -> e2 ---\n";
    Event<int> e2 = std::move(e1);
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
// main
// ============================================================
int main() {
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
    demo_emit_forward_caveat();
    demo_event_value_with_move();
    demo_multi_subscriber_value_safety();

    demo_event_move();

    std::cout << "\n========== All demos done ==========\n";
    return 0;
}
