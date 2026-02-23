#pragma once

/// 此文件保留仅为向后兼容. 所有实现已统一到 eventBus.hpp 中.
/// ThreadSafe* 类型均为 eventBus.hpp 中带 SharedMutexLock 策略的别名.
///
/// 新代码推荐直接使用:
///   evt::Event<Sig, Combiner, evt::SharedMutexLock>  或
///   evt::ThreadSafeEvent<Sig, Combiner>
///   evt::ThreadSafeMessageBus

#include "eventBus.hpp"
