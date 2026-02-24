# eventBus14.hpp — C++14 事件总线库 API 文档

> **文件路径**: `signalDelegate/include/generic/eventBus14.hpp`
>
> **命名空间**: `evt`
>
> **C++ 标准**: C++14（兼容降级自 C++17 版本）

---

## 目录

- [概述](#概述)
- [C++17 → C++14 适配说明](#c17--c14-适配说明)
- [架构总览](#架构总览)
- [锁策略 (Lock Policies)](#锁策略-lock-policies)
  - [NoLock](#nolock)
  - [SharedMutexLock](#sharedmutexlock)
- [ScopedConnection](#scopedconnection)
- [Delegate](#delegate)
- [Combiner 合并策略](#combiner-合并策略)
  - [CollectAll](#collectall)
  - [LastValue](#lastvalue)
  - [StopOnTrue](#stopontrue)
  - [StopOnFalse](#stoponfalse)
  - [自定义 Combiner](#自定义-combiner)
- [Event](#event)
  - [模板参数](#模板参数)
  - [void 返回值特化](#void-返回值特化)
  - [非 void 返回值特化](#非-void-返回值特化)
- [BasicMessageBus](#basicmessagebus)
  - [void 消息通道](#void-消息通道)
  - [非 void 消息通道](#非-void-消息通道)
  - [post / flush 机制](#post--flush-机制)
- [内部实现细节](#内部实现细节)
  - [EventCore (COW 快照)](#eventcore-cow-快照)
  - [detail 工具](#detail-工具)
- [使用示例](#使用示例)
- [线程安全说明](#线程安全说明)
- [异常安全说明](#异常安全说明)

---

## 概述

`eventBus14.hpp` 是一个 **header-only** 的 C++14 事件/信号系统库，提供以下核心功能：

| 特性 | 说明 |
|------|------|
| **类型安全** | 基于模板，编译期检查事件签名 |
| **策略模式** | 通过 `LockPolicy` 模板参数统一单线程/多线程版本 |
| **COW 快照** | emit 期间对 handler 列表做写时复制快照，避免遍历时锁竞争 |
| **RAII 连接管理** | `ScopedConnection` 自动断开订阅，防止悬挂回调 |
| **返回值合并** | 内建多种 Combiner（`CollectAll`、`LastValue`、`StopOnTrue`、`StopOnFalse`），支持短路求值 |
| **延迟派发** | `post()` + `flush()` 支持异步消息队列模式 |
| **消息总线** | `BasicMessageBus` 提供基于类型路由的发布-订阅模式 |

---

## C++17 → C++14 适配说明

| C++17 原始设施 | C++14 替代 |
|---|---|
| `std::optional<T>` | `std::unique_ptr<T>` |
| `std::shared_mutex` | `std::shared_timed_mutex` |
| `std::void_t` | `detail::void_t`（CWG 1558 安全实现） |
| `std::is_same_v<A,B>` | `std::is_same<A,B>::value` |
| `std::is_void_v<T>` | `std::is_void<T>::value` |
| `std::apply` | `detail::apply` |
| 结构化绑定 `auto [k,v]` | `.first` / `.second` |

---

## 架构总览

```
┌─────────────────────────────────────────────────────────┐
│                    BasicMessageBus                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ TypedChannel  │  │ TypedChannel  │  │TypedRetChan. │  │
│  │  <MsgA>       │  │  <MsgB>       │  │ <MsgC,R,Comb>│  │
│  │  ┌─────────┐  │  │  ┌─────────┐  │  │  ┌─────────┐ │  │
│  │  │  Event   │  │  │  │  Event   │  │  │  │  Event  │ │  │
│  │  └─────────┘  │  │  └─────────┘  │  │  └─────────┘ │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘

Event 内部:
┌──────────┐      ┌───────────────┐
│  Event   │─────▶│   EventCore   │
│          │      │ (shared_ptr)  │
└──────────┘      │               │
                  │ m_handlers ──▶ shared_ptr<vector<Slot>>  ← COW 快照
                  │ m_pending  ──▶ vector<StoredArgs>        ← 延迟队列
                  │ m_lock     ──▶ LockPolicy                ← 策略锁
                  └───────────────┘

ScopedConnection:
  构造时接收 disconnect 回调
  析构时自动调用 → 通过 weak_ptr 安全移除 handler
```

---

## 锁策略 (Lock Policies)

锁策略是模板参数，通过鸭子类型（Duck Typing）提供统一接口。所有策略需实现以下四个方法：

```cpp
void lock() const;
void unlock() const;
void lock_shared() const;
void unlock_shared() const;
```

### NoLock

```cpp
struct NoLock;
```

**零开销**的空操作锁策略，用于**单线程**场景。所有方法为 `noexcept` 空函数，编译器可完全优化掉。

### SharedMutexLock

```cpp
struct SharedMutexLock;
```

基于 `std::shared_timed_mutex` 的**读写锁**策略，用于**多线程**场景。

- `lock()` / `unlock()`：独占写锁
- `lock_shared()` / `unlock_shared()`：共享读锁

读操作（如 `emit` 取快照、`handler_count`）使用共享锁；写操作（如 `subscribe`、`remove_handler`）使用独占锁，实现高并发读、低竞争写。

---

## ScopedConnection

```cpp
class ScopedConnection;
```

RAII 风格的连接管理器。当 `ScopedConnection` 对象析构时，自动断开对应的事件订阅。

### 构造 / 析构

| 方法 | 说明 |
|------|------|
| `ScopedConnection()` | 默认构造，无效连接 |
| `ScopedConnection(std::function<void()> disconnect_fn)` | 接收断开回调 |
| `~ScopedConnection()` | 析构时自动调用 `disconnect()` |

### 移动语义

`ScopedConnection` 支持**移动**但**禁止拷贝**。移动时使用 `atomic<bool>` 的 `exchange` 操作保证所有权原子转移。

### 公共方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `disconnect` | `void disconnect()` | 手动断开连接（仅执行一次，线程安全） |
| `is_active` | `bool is_active() const` | 查询连接是否仍处于活跃状态 |

### 使用模式

```cpp
evt::Event<void(int)> event;

{
    auto conn = event.subscribe([](int v) { /* ... */ });
    event.emit(42);  // handler 被调用
}
// conn 析构，handler 自动移除

event.emit(42);  // 无 handler，不执行任何操作
```

---

## Delegate

```cpp
template <typename Signature>
class Delegate;

// 偏特化
template <typename R, typename... Args>
class Delegate<R(Args...)>;
```

类型安全的可调用对象包装器，内部基于 `std::function`。

### 静态工厂方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `from(F&&)` | `template<typename F> static Delegate from(F&& func)` | 从任意可调用对象构造 |
| `from(func_ptr)` | `static Delegate from(R(*func)(Args...))` | 从函数指针构造 |
| `from(obj, method)` | `template<typename T> static Delegate from(T* obj, R(T::*method)(Args...))` | 从对象 + 成员函数指针构造 |
| `from(obj, const_method)` | `template<typename T> static Delegate from(const T* obj, R(T::*method)(Args...) const)` | 从对象 + const 成员函数指针构造 |

### 公共方法

| 方法 | 签名 | 说明 |
|------|------|------|
| `operator()` | `R operator()(Args... args) const` | 调用包装的函数（空 delegate 触发 assert） |
| `valid` | `bool valid() const` | 检查是否持有有效的可调用对象 |

### 使用示例

```cpp
// 从 lambda
auto d1 = evt::Delegate<int(int, int)>::from([](int a, int b) { return a + b; });

// 从函数指针
int add(int a, int b) { return a + b; }
auto d2 = evt::Delegate<int(int, int)>::from(&add);

// 从成员函数
struct Calc {
    int multiply(int a, int b) { return a * b; }
};
Calc calc;
auto d3 = evt::Delegate<int(int, int)>::from(&calc, &Calc::multiply);

d1(3, 4);  // 返回 7
```

---

## Combiner 合并策略

Combiner 用于非 void 返回值的事件，决定如何合并多个 handler 的返回值。

### CollectAll

```cpp
template <typename R>
struct CollectAll;
```

**默认 Combiner**。收集所有 handler 的返回值到 `std::vector<R>`。

| 类型 / 方法 | 说明 |
|---|---|
| `result_type` = `std::vector<R>` | 合并后的结果类型 |
| `empty_result()` | 无 handler 时返回空 `vector` |
| `combine(vector<R>&&)` | 直接返回收集到的结果 |

### LastValue

```cpp
template <typename R>
struct LastValue;
```

只保留**最后一个** handler 的返回值。

| 类型 / 方法 | 说明 |
|---|---|
| `result_type` = `R` | 合并后的结果类型 |
| `Accumulator` | 自定义累加器，内部使用 `unique_ptr<R>` 保存最后结果 |

> **注意**：至少需要一个 handler，否则 `finalize()` 触发 assert。

### StopOnTrue

```cpp
template <typename R>
struct StopOnTrue;
```

**短路求值**：遇到返回值为 `true`（可隐式转换为 bool）的 handler 立即停止遍历。

| 类型 / 方法 | 说明 |
|---|---|
| `result_type` = `R` | 合并后的结果类型 |
| `Accumulator::should_stop()` | 当 `*last_value` 为 true 时返回 true，触发短路 |

### StopOnFalse

```cpp
template <typename R>
struct StopOnFalse;
```

**短路求值**：遇到返回值为 `false` 的 handler 立即停止遍历。

| 类型 / 方法 | 说明 |
|---|---|
| `result_type` = `R` | 合并后的结果类型 |
| `Accumulator::should_stop()` | 当 `!*last_value` 为 true 时返回 true，触发短路 |

### 自定义 Combiner

自定义 Combiner 需满足以下接口约定：

```cpp
template <typename R>
struct MyCombiner {
    using result_type = /* 合并后的类型 */;

    // [可选] 无 handler 时的默认返回值
    static result_type empty_result();

    // 方式一：简单合并（库自动收集到 vector 后传入）
    static result_type combine(std::vector<R>&& results);

    // 方式二：自定义 Accumulator（支持短路求值，优先级高于 combine）
    struct Accumulator {
        void reserve(std::size_t n);       // 预分配
        void add(R&& val);                 // 接收单个结果
        bool should_stop() const;          // 是否短路停止
        result_type finalize();            // 返回最终结果
    };
};
```

- 若同时定义了 `Accumulator` 和 `combine`，库优先使用 `Accumulator`。
- 若未定义 `empty_result()`，在无 handler 时 emit 将抛出 `std::logic_error`。

---

## Event

```cpp
template <
    typename Signature,                              // 函数签名，如 void(int) 或 int(string)
    template <typename> class CombinerT = CollectAll, // 合并策略（仅非 void 返回值有效）
    typename LockPolicy = NoLock                      // 锁策略
>
class Event;
```

核心事件类，支持两个特化版本：

### 模板参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `Signature` | 事件的函数签名 | — |
| `CombinerT` | 返回值合并策略模板 | `CollectAll` |
| `LockPolicy` | 线程安全策略 | `NoLock` |

### void 返回值特化

```cpp
Event<void(Args...), CombinerT, LockPolicy>
```

#### 类型别名

| 名称 | 定义 |
|------|------|
| `Handler` | `Delegate<void(Args...)>` |
| `Id` | `std::size_t` |

#### 公共方法

| 方法 | 签名 | 说明 |
|------|------|------|
| **subscribe** | `ScopedConnection subscribe(Handler handler)` | 注册 handler，返回 RAII 连接 |
| **subscribe** | `template<typename F> ScopedConnection subscribe(F&& func)` | 从可调用对象注册 |
| **emit** | `void emit(Args... args) const` | 触发事件，COW 快照遍历 |
| **operator()** | `void operator()(Args... args) const` | `emit` 的简写 |
| **clear** | `void clear()` | 移除所有 handler |
| **size** | `std::size_t size() const` | 当前 handler 数量 |
| **empty** | `bool empty() const` | 是否无 handler |
| **post** | `template<typename... UArgs> void post(UArgs&&... uargs)` | 入队延迟事件 |
| **flush** | `void flush()` | 派发所有挂起事件 |
| **pending_count** | `std::size_t pending_count() const` | 挂起事件数量 |
| **clear_pending** | `void clear_pending()` | 清除所有挂起事件 |

### 非 void 返回值特化

```cpp
Event<R(Args...), CombinerT, LockPolicy>
```

#### 类型别名

| 名称 | 定义 |
|------|------|
| `Handler` | `Delegate<R(Args...)>` |
| `Id` | `std::size_t` |
| `Combiner` | `CombinerT<R>` |
| `ResultType` | `typename Combiner::result_type` |

#### 公共方法

| 方法 | 签名 | 说明 |
|------|------|------|
| **subscribe** | `ScopedConnection subscribe(Handler handler)` | 注册 handler |
| **subscribe** | `template<typename F> ScopedConnection subscribe(F&& func)` | 从可调用对象注册 |
| **emit** | `ResultType emit(Args... args) const` | 触发事件并返回合并结果 |
| **operator()** | `ResultType operator()(Args... args) const` | `emit` 的简写 |
| **clear** | `void clear()` | 移除所有 handler |
| **size** | `std::size_t size() const` | 当前 handler 数量 |
| **empty** | `bool empty() const` | 是否无 handler |
| **post** | `template<typename... UArgs> void post(UArgs&&... uargs)` | 入队延迟事件 |
| **flush** | `std::vector<ResultType> flush()` | 派发所有挂起事件，返回每批合并结果 |
| **pending_count** | `std::size_t pending_count() const` | 挂起事件数量 |
| **clear_pending** | `void clear_pending()` | 清除所有挂起事件 |

#### emit 短路求值流程

```
1. 取 COW 快照（共享锁 O(1)）
2. 若快照为空 → 返回 empty_result()
3. 创建 Accumulator
4. 遍历快照中的每个 slot：
   ├─ 调用 handler → acc.add(result)
   ├─ 若 acc.should_stop() → break（短路）
   └─ 若异常 → 捕获并保留第一个异常
5. 若有捕获的异常 → rethrow
6. 返回 acc.finalize()
```

### 移动语义

`Event` 支持**移动**但**禁止拷贝**。移动后原 Event 的 `m_core` 被转移，原对象变为空。

---

## BasicMessageBus

```cpp
template <typename LockPolicy = NoLock>
class BasicMessageBus;
```

基于**消息类型**自动路由的发布-订阅总线。内部使用 `std::type_index` 作为 channel key，支持同类型 void 通道和非 void 通道共存。

### 常用类型别名

```cpp
using MessageBus           = BasicMessageBus<NoLock>;           // 单线程
using MessageBusThreadSafe = BasicMessageBus<SharedMutexLock>;  // 多线程
```

### void 消息通道

| 方法 | 签名 | 说明 |
|------|------|------|
| **subscribe** | `template<typename Message> ScopedConnection subscribe(std::function<void(const Message&)> func)` | 订阅消息 |
| **subscribe** | `template<typename Message, typename F> ScopedConnection subscribe(F&& func)` | 从可调用对象订阅 |
| **emit** | `template<typename Message> void emit(const Message& message) const` | 发布消息 |
| **clear** | `template<typename Message> void clear()` | 清除指定消息类型的所有订阅 |

### 非 void 消息通道

| 方法 | 签名 | 说明 |
|------|------|------|
| **subscribe** | `template<typename Message, typename R, template<typename> class CombinerTC> ScopedConnection subscribe(std::function<R(const Message&)> func)` | 订阅带返回值的消息 |
| **emit** | `template<typename Message, typename R, template<typename> class CombinerTC> typename CombinerTC<R>::result_type emit(const Message& message) const` | 发布并获取合并返回值 |
| **clear** | `template<typename Message, typename R, template<typename> class CombinerTC> void clear()` | 清除指定通道 |

### post / flush 机制

`post()` 将消息入队，`flush()` 统一派发。两种使用方式：

**1. 通用 flush（void 通道）**

```cpp
bus.post(MyMessage{42});
bus.flush();  // 派发所有通道的挂起消息
```

**2. 类型化 flush（非 void 通道）**

```cpp
bus.post<MyMessage, int>(MyMessage{42});
auto results = bus.flush<MyMessage, int>();  // 返回 vector<vector<int>>
```

**3. 取上次 flush 结果**

对于非 void 通道，通用 `flush()` 会缓存返回值，可通过 `take_flush_results` 获取：

```cpp
bus.flush();  // 通用 flush
auto results = bus.take_flush_results<MyMessage, int>();  // 仅取一次，move 语义
```

| 方法 | 签名 | 说明 |
|------|------|------|
| **post** | `template<typename Message> void post(Message&& message)` | void 通道入队 |
| **post** | `template<typename Message, typename R, ...> void post(const Message& message)` | 非 void 通道入队 |
| **flush** | `void flush()` | 派发所有通道 |
| **flush** | `template<typename Message, typename R, ...> std::vector<ResultType> flush()` | 派发指定通道并返回结果 |
| **take_flush_results** | `template<typename Message, typename R, ...> std::vector<ResultType> take_flush_results()` | 获取上次通用 flush 缓存的结果 |
| **pending_count** | `std::size_t pending_count() const` | 所有通道的挂起消息总数 |

### 移动语义

- `LockPolicy = NoLock` 时支持移动（单线程版可转移所有权）
- `LockPolicy = SharedMutexLock` 时禁止移动（`shared_timed_mutex` 不可移动）

### static_assert 约束

所有接收 `Message` 类型参数的方法均包含：

```cpp
static_assert(std::is_same<Message, std::decay_t<Message>>::value,
              "Message type must not be cv-qualified or a reference.");
```

确保消息类型不带 cv 限定符或引用。

---

## 内部实现细节

### EventCore (COW 快照)

```cpp
namespace detail {
template <typename Handler, typename LockPolicy, typename... Args>
class EventCore;
}
```

`Event` 的内部共享状态，通过 `std::shared_ptr` 持有。

#### COW (Copy-on-Write) 机制

- `m_handlers` 是 `shared_ptr<vector<Slot>>`
- `snapshot()` 仅增加引用计数（O(1)），返回指向当前 handler 列表的共享指针
- `add_handler()` / `remove_handler()` 时调用 `cow_detach()`：若引用计数 > 1，则先复制一份再修改
- **效果**：emit 遍历期间，subscribe/disconnect 不会影响正在遍历的快照，也无需在整个遍历期间持有锁

```
emit 线程                          subscribe 线程
─────────                          ──────────────
snap = snapshot()    ─── shared_ptr refcount = 2
遍历 snap 中的 handler                cow_detach() → 复制新列表
                                     修改新列表
遍历不受影响                           新的 subscribe 进入新列表
```

#### 延迟派发 (Pending)

- `push_pending(args...)` 将参数打包为 `tuple` 存入 `m_pending`
- `take_pending()` 以 move 语义取出所有挂起的参数包
- `flush` 时逐个解包并调用 `emit`

### detail 工具

| 工具 | 说明 |
|------|------|
| `UniqueLockGuard<L>` | RAII 独占锁守卫，构造时 `lock()`，析构时 `unlock()` |
| `SharedLockGuard<L>` | RAII 共享锁守卫，构造时 `lock_shared()`，析构时 `unlock_shared()` |
| `void_t<Ts...>` | CWG 1558 安全的 `void_t` 实现 |
| `MovePolicy<bool>` | 条件性启用/禁用移动构造和移动赋值 |
| `for_each_safe` | 异常安全遍历：捕获并保留第一个异常，遍历完成后重新抛出 |
| `apply(f, tuple)` | C++14 版 `std::apply`，展开 tuple 作为函数参数 |
| `HasAccumulator<C,R>` | SFINAE 检测 Combiner 是否定义了 `Accumulator` 类型 |
| `CombinerAdapter` | 为没有自定义 `Accumulator` 的 Combiner 生成默认 Accumulator |
| `HasEmptyResult<C>` | SFINAE 检测 Combiner 是否定义了 `empty_result()` |
| `EmptyResultAdapter` | 无 `empty_result()` 时抛出 `std::logic_error` |

---

## 使用示例

### 基本 void 事件

```cpp
#include "eventBus14.hpp"

evt::Event<void(int, const std::string&)> onDataReceived;

auto conn = onDataReceived.subscribe([](int id, const std::string& data) {
    std::cout << "Received #" << id << ": " << data << std::endl;
});

onDataReceived.emit(1, "hello");  // 输出: Received #1: hello
conn.disconnect();                // 手动断开（也可依赖析构）
```

### 带返回值事件 + Combiner

```cpp
// 收集所有返回值
evt::Event<int(int)> computeEvent;
auto c1 = computeEvent.subscribe([](int x) { return x * 2; });
auto c2 = computeEvent.subscribe([](int x) { return x + 10; });

std::vector<int> results = computeEvent.emit(5);
// results = {10, 15}

// 短路求值：仅取最后一个值
evt::Event<int(int), evt::LastValue> lastEvent;
auto c3 = lastEvent.subscribe([](int x) { return x * 2; });
auto c4 = lastEvent.subscribe([](int x) { return x + 10; });
int last = lastEvent.emit(5);
// last = 15

// 短路求值：遇到 true 即停止
evt::Event<bool(int), evt::StopOnTrue> guardEvent;
auto g1 = guardEvent.subscribe([](int x) { return x > 10; });
auto g2 = guardEvent.subscribe([](int x) { return x > 5; });  // x=7 时不会被调用
bool stopped = guardEvent.emit(7);
// stopped = false (g1 返回 false, g2 返回 true → 短路)
```

### 延迟派发 (post/flush)

```cpp
evt::Event<void(int)> event;
auto conn = event.subscribe([](int v) { std::cout << v << " "; });

event.post(1);
event.post(2);
event.post(3);

std::cout << "pending: " << event.pending_count() << std::endl;  // 3
event.flush();  // 输出: 1 2 3
```

### 消息总线

```cpp
struct PlayerDied {
    int player_id;
};

struct DamageDealt {
    int target_id;
    float amount;
};

evt::BasicMessageBus<> bus;

auto conn1 = bus.subscribe<PlayerDied>([](const PlayerDied& msg) {
    std::cout << "Player " << msg.player_id << " died!" << std::endl;
});

auto conn2 = bus.subscribe<DamageDealt>([](const DamageDealt& msg) {
    std::cout << "Target " << msg.target_id << " took " << msg.amount << " damage" << std::endl;
});

bus.emit(PlayerDied{42});
bus.emit(DamageDealt{7, 99.5f});
```

### 多线程安全版本

```cpp
// 线程安全 Event
evt::Event<void(int), evt::CollectAll, evt::SharedMutexLock> safeEvent;

// 线程安全 MessageBus
evt::BasicMessageBus<evt::SharedMutexLock> safeBus;
```

### 成员函数绑定

```cpp
struct AudioSystem {
    void onExplosion(float x, float y) {
        std::cout << "Boom at (" << x << ", " << y << ")" << std::endl;
    }
};

evt::Event<void(float, float)> explosionEvent;
AudioSystem audio;

auto conn = explosionEvent.subscribe(
    evt::Delegate<void(float, float)>::from(&audio, &AudioSystem::onExplosion)
);

explosionEvent.emit(10.0f, 20.0f);
```

---

## 线程安全说明

| 组件 | `NoLock` | `SharedMutexLock` |
|------|----------|-------------------|
| `Event` | 非线程安全 | 线程安全（读写分离） |
| `BasicMessageBus` | 非线程安全 | 线程安全 |
| `ScopedConnection` | 线程安全（`atomic<bool>`） | 线程安全 |
| `Delegate` | 非线程安全（通常不跨线程共享） | — |

**关键保证**（使用 `SharedMutexLock` 时）：

1. **subscribe / disconnect 与 emit 并发安全**：emit 使用 COW 快照，不会与修改操作冲突
2. **disconnect 仅执行一次**：`ScopedConnection` 使用 `atomic<bool>` 保证幂等性
3. **flush 期间新 post 不会在本次 flush 中派发**：`take_pending()` 先 move 出所有挂起事件

---

## 异常安全说明

- **emit 遍历**使用 `for_each_safe`：某个 handler 抛异常不会阻止后续 handler 执行
- 第一个异常被保留，遍历完成后**重新抛出**
- `ScopedConnection` 析构函数中 `disconnect` 的异常被吞掉（`noexcept` 析构）
- COW 快照确保即使 handler 中调用 subscribe/disconnect 也不会导致迭代器失效
