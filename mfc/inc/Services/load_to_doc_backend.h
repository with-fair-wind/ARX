#pragma once

#include <string>
#include <vector>

struct LoadToDocTarget {
    std::wstring id;
    std::wstring name;
    bool is_current = false;
};

struct RenameResult {
    bool accepted = false;
    bool cancel_load = false;
    std::wstring new_name;
};

// 获取可载入的文档列表（由后端实现，需排除当前文档或标记 is_current）
std::vector<LoadToDocTarget> getLoadToDocTargets();

// 执行载入逻辑（由后端实现）
bool executeLoadToDocuments(const std::vector<std::wstring>& doc_ids, bool place_after_load, bool close_after_load, std::wstring* error_message);

// 弹出同名构件重命名对话框，返回用户的选择和新名称
RenameResult showRenameDialog(const std::wstring& original_name);

// 弹出同名构件警告提示框，返回用户是否选择继续载入
bool showDuplicateWarning(const std::wstring& template_name);
