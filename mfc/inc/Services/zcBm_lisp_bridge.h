#pragma once

#include <acedads.h>

using ZcBmLispRawHandler = int (*)(resbuf*);

int ZcBmRegisterLispCommands();
int ZcBmUnregisterLispCommands();
int ZcBmDispatchLispCommand();

// 运行时扩展/移除 Lisp 函数（name -> handler -> funcCode）
int ZcBmAddLispFunction(const ZTCHAR* function_name, ZcBmLispRawHandler handler, int* out_func_code = nullptr);
int ZcBmRemoveLispFunction(const ZTCHAR* function_name);
