#pragma once

/// ZcBm 专用事件总线 — 全部委托给 generic/eventBus.hpp 的实现.
/// 仅提供 ZcBm 前缀的别名, 消除代码复制.

#include "generic/eventBus.hpp"

namespace evt {

// ---- Connection ----
using ZcBmScopedConnection = ScopedConnection;

// ---- Delegate ----
template <typename Signature>
using ZcBmDelegate = Delegate<Signature>;

// ---- Combiners ----
template <typename R>
using ZcBmCollectAll = CollectAll<R>;
template <typename R>
using ZcBmLastValue = LastValue<R>;

// ---- Event (非线程安全, 与 generic Event 完全一致) ----
template <typename Signature, template <typename> class CombinerT = CollectAll>
using ZcBmEvent = Event<Signature, CombinerT, NoLock>;

// ---- MessageBus ----
using ZcBmMessageBus = BasicMessageBus<>;

}  // namespace evt
