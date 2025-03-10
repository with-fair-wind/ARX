#pragma once

#ifdef KK_OBJECTARX
#define KK_OBJECTARX_API __declspec(dllexport)
#else
#define KK_OBJECTARX_API __declspec(dllimport)
#endif

#include <kkArxUtility.h>

#ifndef KK_OBJECTARX
// 相当于在vs配置中添加 附加依赖项
#pragma comment(lib, "ArxApi.lib")
#endif