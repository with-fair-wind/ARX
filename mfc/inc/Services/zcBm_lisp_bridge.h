#pragma once

#include <acedads.h>

using ZcBmLispRawHandler = int (*)(resbuf*);
static const int kZcBmLispDispatchNotHandled = 1;

int ZcBmRegisterLispCommands();
int ZcBmUnregisterLispCommands();
// 返回值:
// - RTNORM: 已由本框架处理成功
// - RTERROR: 命中本框架函数但处理失败
// - kZcBmLispDispatchNotHandled: 非本框架 funcCode，应由外层继续分发
int ZcBmDispatchLispCommand();

// 运行时扩展/移除 Lisp 函数（name -> handler -> funcCode）
int ZcBmAddLispFunction(const ZTCHAR* function_name, ZcBmLispRawHandler handler, int* out_func_code = nullptr);
int ZcBmRemoveLispFunction(const ZTCHAR* function_name);
