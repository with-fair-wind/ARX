#pragma once

#include <string>
#include <vector>

struct LoadToDocTarget {
    std::wstring id;
    std::wstring name;
    bool is_current = false;
};

// 获取可载入的文档列表（由后端实现，需排除当前文档或标记 is_current）
std::vector<LoadToDocTarget> getLoadToDocTargets();

// 执行载入逻辑（由后端实现）
bool executeLoadToDocuments(const std::vector<std::wstring>& doc_ids, bool place_after_load, bool close_after_load, std::wstring* error_message);
