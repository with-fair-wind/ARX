#pragma once

#ifdef KT_EXPORTARX
#define KT_EXPORTARX_API __declspec(dllexport)
#else
#define KT_EXPORTARX_API __declspec(dllimport)
#endif

#ifndef DEL
#define DEL(ptr)         \
    if (ptr)             \
    {                    \
        delete (ptr);    \
        (ptr) = nullptr; \
    }
#endif

#include <KTArxTool/KTArxUtility.h>

#ifndef KT_EXPORTARX
// 相当于在vs配置中添加 附加依赖项
#pragma comment(lib, "arxApi.lib")
#endif