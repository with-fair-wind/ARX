#pragma once

#include <string>

/// 加载族模板到当前文档
/// 内部会通过 AppEventService::defer() 延迟通知 UI
bool loadTemplate(const std::wstring& template_name);

/// 卸载族模板
bool unloadTemplate(const std::wstring& template_name);
