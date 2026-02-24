#pragma once

/// ZcBm 专用线程安全事件总线 — 全部委托给 generic/eventBus.hpp 的实现.
/// 仅提供 ZcBmTs 前缀的别名, 消除代码复制.

#include "generic/eventBus.hpp"

namespace evt {

// ---- Connection ----
using ZcBmTsScopedConnection = ScopedConnection;

// ---- Delegate ----
template <typename Signature>
using ZcBmTsDelegate = Delegate<Signature>;

// ---- Combiners ----
template <typename R>
using ZcBmTsCollectAll = CollectAll<R>;
template <typename R>
using ZcBmTsLastValue = LastValue<R>;

// ---- Event (线程安全) ----
template <typename Signature, template <typename> class CombinerT = CollectAll>
using ZcBmTsEvent = Event<Signature, CombinerT, SharedMutexLock>;

// ---- MessageBus (线程安全) ----
using ZcBmTsMessageBus = BasicMessageBus<SharedMutexLock>;

}  // namespace evt
