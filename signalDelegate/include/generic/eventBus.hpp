#pragma once

/// 分发头文件: 根据 CMake 选项自动选择 C++14 或 C++17 实现.
///
/// CMake 用法:
///   option(EVT_USE_CPP14 "Use C++14 event bus implementation" OFF)
///   cmake -DEVT_USE_CPP14=ON ..
///
/// 也可手动 #define EVT_USE_CPP14 在包含此头文件之前.

#if defined(EVT_USE_CPP14)
#include "eventBus14.hpp"
#else
#include "eventBus17.hpp"
#endif
