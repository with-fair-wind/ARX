#pragma once

/// 此文件保留仅为向后兼容. 所有实现已统一到 eventBus.hpp 中.
/// eventBus.hpp 是分发头文件, 根据 EVT_USE_CPP14 宏自动选择 C++14/17 实现.
/// ThreadSafe* 类型均为带 SharedMutexLock 策略的别名.
///
/// 新代码推荐直接使用:
///   evt::Event<Sig, Combiner, evt::SharedMutexLock>  或
///   evt::ThreadSafeEvent<Sig, Combiner>
///   evt::ThreadSafeMessageBus

#include "eventBus.hpp"
