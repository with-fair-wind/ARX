# delegate_event_cpp14

一个兼容 **C++14** 的通用事件/消息机制库，目标是提供接近 C# 委托 + 事件的使用体验。

纯头文件实现，只需 `#include "event_system.hpp"` 即可使用，零外部依赖。

## 功能概览

| 组件 | 说明 |
|---|---|
| `Delegate<R(Args...)>` | 统一封装自由函数、成员函数、lambda，等价于 C# 的 `delegate` |
| `Event<Args...>` | 事件发布/订阅容器，支持多个监听器，等价于 C# 的 `event` |
| `ScopedConnection` | RAII 连接句柄，析构时自动取消订阅，也可手动 `disconnect()` |
| `MessageBus` | 按消息类型订阅/发布的全局总线，适合模块间解耦通信 |

## 项目结构

```
delegate_event_cpp14/
├── CMakeLists.txt            # 构建配置（C++14）
├── README.md
├── include/
│   └── event_system.hpp      # 核心实现（纯头文件）
└── src/
    └── main.cpp              # 使用示例
```

## 构建与运行

```bash
cmake -S . -B build
cmake --build build

# Linux / macOS
./build/delegate_event_demo

# Windows (MSVC)
.\build\Debug\delegate_event_demo.exe
```

## 快速上手

### 1. Delegate — 委托

将不同类型的可调用体统一为同一签名：

```cpp
#include "event_system.hpp"

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

```cpp
evt::Event<int, int> on_click;

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

### 3. Event + 成员函数

```cpp
struct Player {
    void on_damage(int value) {
        hp -= value;
        std::cout << "Player hp=" << hp << "\n";
    }
    int hp = 100;
};

evt::Event<int> damage_event;
Player player;

// 绑定成员函数
auto conn = damage_event.subscribe(
    evt::Delegate<void(int)>::from(&player, &Player::on_damage)
);

damage_event.emit(25);  // Player hp=75
```

### 4. MessageBus — 消息总线（按类型路由）

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

// 按类型订阅
auto conn1 = bus.subscribe<DamageEvent>([](const DamageEvent& e) {
    std::cout << "damage: " << e.amount << " from " << e.source << "\n";
});

auto conn2 = bus.subscribe<LoginEvent>([](const LoginEvent& e) {
    std::cout << "login: " << e.user << "\n";
});

// 发布 — 只有对应类型的订阅者会收到
bus.publish(DamageEvent{30, "fire"});
bus.publish(LoginEvent{"bob"});
```

## 与 C# 委托/事件的对照

| C# | 本库 | 说明 |
|---|---|---|
| `delegate void MyHandler(int x)` | `Delegate<void(int)>` | 声明可调用签名 |
| `new MyHandler(obj.Method)` | `Delegate<void(int)>::from(&obj, &Obj::Method)` | 绑定成员函数 |
| `event MyHandler OnDamage` | `Event<int> on_damage` | 声明事件 |
| `OnDamage += handler` | `auto conn = on_damage.subscribe(handler)` | 订阅，返回连接句柄 |
| `OnDamage -= handler` | `conn.disconnect()` | 取消订阅 |
| `OnDamage?.Invoke(12)` | `on_damage.emit(12)` 或 `on_damage(12)` | 触发事件 |
| `IDisposable` | `ScopedConnection` 析构自动解绑 | RAII 比 C# 更安全 |
| `EventAggregator` | `MessageBus` | 按类型解耦的全局消息 |

## 设计说明

### 纯头文件

整个实现在 `include/event_system.hpp` 中完成，任何 C++14 项目只需 `#include` 即可，无需链接额外库。

### 快照遍历

`Event::emit()` 内部会先拷贝一份 handler 列表再遍历。这意味着在回调执行过程中可以安全地调用 `subscribe()` 或 `disconnect()`，不会导致迭代器失效。

### weak_ptr 防悬空

`ScopedConnection` 的断开回调内部持有 `weak_ptr<HandlerList>`。即使 `Event` 对象先于 `ScopedConnection` 销毁，断开操作也不会访问已释放的内存。

### 线程安全

当前实现为**单线程语义**，不含锁。若需在多线程环境下使用，建议：

- 在 `Event` 的 `subscribe()` / `emit()` 外围加 `std::mutex`
- 或派生一个 `ThreadSafeEvent` 封装锁逻辑

### 在已有项目中集成

只需两步：

1. 将 `include/event_system.hpp` 拷贝到你的项目中
2. 在需要的地方 `#include "event_system.hpp"`

或者作为 CMake 子项目：

```cmake
# 在你的 CMakeLists.txt 中
add_subdirectory(delegate_event_cpp14)
target_include_directories(your_target PRIVATE delegate_event_cpp14/include)
```

## 许可

自由使用，无限制。
