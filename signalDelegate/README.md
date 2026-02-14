# delegate_event_cpp14

一个兼容 **C++14** 的通用事件/消息机制库，目标是提供接近 C# 委托 + 事件的使用体验。

纯头文件实现，按使用场景选择 include 路径即可，零外部依赖。

## 功能概览

| 组件 | 说明 |
|---|---|
| `Delegate<R(Args...)>` | 统一封装自由函数、成员函数、lambda，等价于 C# 的 `delegate` |
| `Event<R(Args...), CombinerT>` | 事件发布/订阅容器，支持多个监听器和返回值合并，等价于 C# 的 `event` |
| `CollectAll<R>` / `LastValue<R>` | 内置合并策略：收集所有返回值 / 取最后一个返回值 |
| `ScopedConnection` | RAII 连接句柄，析构时自动取消订阅，也可手动 `disconnect()` |
| `MessageBus` | 按消息类型订阅/发布的全局总线，支持 void 和非 void 返回值 |

## 项目结构

```
delegate_event_cpp14/
├── CMakeLists.txt            # 构建配置（C++14）
├── README.md
├── include/
│   ├── generic/
│   │   ├── event_system.hpp
│   │   └── event_system_threadsafe.hpp
│   └── zcbm/
│       ├── event_system.hpp
│       └── event_system_threadsafe.hpp
└── demo/
    ├── main.cpp
    └── main_threadsafe.cpp
```

## 构建与运行

```bash
cmake -S . -B build
cmake --build build

# Linux / macOS
./build/delegate_event_demo
./build/delegate_event_threadsafe_demo

# Windows (MSVC)
.\build\Debug\delegate_event_demo.exe
.\build\Debug\delegate_event_threadsafe_demo.exe
```

## 快速上手

### 1. Delegate — 委托

将不同类型的可调用体统一为同一签名：

```cpp
#include "generic/event_system.hpp"

// 自由函数
void on_click(int x, int y) { /* ... */ }
auto d1 = evt::Delegate<void(int, int)>::from(&on_click);

// Lambda
auto d2 = evt::Delegate<void(int, int)>::from([](int x, int y) { /* ... */ });

// 成员函数
struct Button {
    void handle_click(int x, int y) { /* ... */ }
};
Button btn;
auto d3 = evt::Delegate<void(int, int)>::from(&btn, &Button::handle_click);

// 调用
d1(10, 20);
d2(10, 20);
d3(10, 20);
```

### 2. Event — 事件（一对多广播）

Event 使用函数签名语法 `Event<R(Args...), CombinerT>`。`R = void` 时行为与传统事件完全一致：

```cpp
evt::Event<void(int, int)> on_click;

// 订阅（返回 RAII 连接句柄）
auto conn1 = on_click.subscribe([](int x, int y) {
    std::cout << "listener1: " << x << ", " << y << "\n";
});

auto conn2 = on_click.subscribe([](int x, int y) {
    std::cout << "listener2: " << x << ", " << y << "\n";
});

// 触发 — 所有订阅者都会收到
on_click.emit(100, 200);
// 也可以用函数调用语法
on_click(100, 200);

// 手动取消某个订阅
conn1.disconnect();

// conn2 出作用域时自动取消订阅（RAII）
```

### 3. Event + 返回值 (Combiner)

当 `R != void` 时，`emit()` 收集所有 handler 的返回值并通过 Combiner 合并：

```cpp
// CollectAll (默认): 返回 std::vector<int>
evt::Event<int(int, int)> compute;
auto c1 = compute.subscribe([](int a, int b) { return a + b; });
auto c2 = compute.subscribe([](int a, int b) { return a * b; });
auto results = compute.emit(3, 4);  // results = {7, 12}

// LastValue: 返回最后一个 handler 的值
evt::Event<int(int, int), evt::LastValue> last;
auto c3 = last.subscribe([](int a, int b) { return a + b; });
auto c4 = last.subscribe([](int a, int b) { return a * b; });
int val = last.emit(3, 4);  // val = 12

// 自定义 Combiner: 只需定义 result_type 和 static combine()
template <typename R>
struct SumCombiner {
    using result_type = R;
    static result_type combine(std::vector<R>&& results) {
        R sum{};
        for (auto& v : results) sum += v;
        return sum;
    }
};
evt::Event<int(int, int), SumCombiner> sum_event;
```

### 4. Event + 成员函数

```cpp
struct Player {
    void on_damage(int value) {
        hp -= value;
        std::cout << "Player hp=" << hp << "\n";
    }
    int hp = 100;
};

evt::Event<void(int)> damage_event;
Player player;

// 绑定成员函数
auto conn = damage_event.subscribe(
    evt::Delegate<void(int)>::from(&player, &Player::on_damage)
);

damage_event.emit(25);  // Player hp=75
```

### 5. MessageBus — 消息总线（按类型路由）

适合模块间不直接持有对方引用时的解耦通信：

```cpp
// 定义消息类型（普通 struct 即可）
struct DamageEvent {
    int amount;
    std::string source;
};

struct LoginEvent {
    std::string user;
};

evt::MessageBus bus;

// 按类型订阅 (void 返回, 1 个模板参数)
auto conn1 = bus.subscribe<DamageEvent>([](const DamageEvent& e) {
    std::cout << "damage: " << e.amount << " from " << e.source << "\n";
});

auto conn2 = bus.subscribe<LoginEvent>([](const LoginEvent& e) {
    std::cout << "login: " << e.user << "\n";
});

// 发布 — 只有对应类型的订阅者会收到
bus.emit(DamageEvent{30, "fire"});
bus.emit(LoginEvent{"bob"});

// 非 void 返回 (2+ 个模板参数: Message, R, [CombinerT])
auto conn3 = bus.subscribe<DamageEvent, int>(
    [](const DamageEvent& e) -> int { return e.amount * 2; }
);
auto results = bus.emit<DamageEvent, int>(DamageEvent{30, "fire"});
// results = std::vector<int>{60}
```

## 与 C# 委托/事件的对照

| C# | 本库 | 说明 |
|---|---|---|
| `delegate void MyHandler(int x)` | `Delegate<void(int)>` | 声明可调用签名 |
| `new MyHandler(obj.Method)` | `Delegate<void(int)>::from(&obj, &Obj::Method)` | 绑定成员函数 |
| `event MyHandler OnDamage` | `Event<void(int)> on_damage` | 声明事件 |
| `OnDamage += handler` | `auto conn = on_damage.subscribe(handler)` | 订阅，返回连接句柄 |
| `OnDamage -= handler` | `conn.disconnect()` | 取消订阅 |
| `OnDamage?.Invoke(12)` | `on_damage.emit(12)` 或 `on_damage(12)` | 触发事件 |
| `IDisposable` | `ScopedConnection` 析构自动解绑 | RAII 比 C# 更安全 |
| `EventAggregator` | `MessageBus` | 按类型解耦的全局消息 |

## 设计说明

### 纯头文件

核心实现为头文件模式，可按场景选择：

- 通用版：`include/generic/event_system.hpp`
- 通用线程安全版：`include/generic/event_system_threadsafe.hpp`
- 内网 ZcBm 版：`include/zcbm/event_system.hpp`
- 内网 ZcBm 线程安全版：`include/zcbm/event_system_threadsafe.hpp`

### 快照遍历

`Event::emit()` 内部会先拷贝一份 handler 列表再遍历。这意味着在回调执行过程中可以安全地调用 `subscribe()` 或 `disconnect()`，不会导致迭代器失效。

### weak_ptr 防悬空

`ScopedConnection` 的断开回调内部持有 `weak_ptr<HandlerList>`。即使 `Event` 对象先于 `ScopedConnection` 销毁，断开操作也不会访问已释放的内存。

### 线程安全

提供了独立线程安全头文件：

- `generic/event_system_threadsafe.hpp`
- `zcbm/event_system_threadsafe.hpp`

### 在已有项目中集成

只需两步：

1. 将 `include/generic/` 或 `include/zcbm/` 拷贝到你的项目中
2. 在需要的地方 `#include "generic/event_system.hpp"`（或对应线程安全/ZcBm版本）

或者作为 CMake 子项目：

```cmake
# 在你的 CMakeLists.txt 中
add_subdirectory(delegate_event_cpp14)
target_include_directories(your_target PRIVATE delegate_event_cpp14/include)
```

## 许可

自由使用，无限制。
