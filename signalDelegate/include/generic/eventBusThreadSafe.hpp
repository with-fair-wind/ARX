#pragma once

/// 此文件保留仅为向后兼容. 所有实现已统一到 eventBus.hpp 中.
/// eventBus.hpp 是分发头文件, 根据 EVT_USE_CPP14 宏自动选择 C++14/17 实现.
///
/// 新代码推荐直接使用:
///   evt::Event<Sig, Combiner, evt::SharedMutexLock>
///   evt::BasicMessageBus<evt::SharedMutexLock>

#include "eventBus.hpp"
