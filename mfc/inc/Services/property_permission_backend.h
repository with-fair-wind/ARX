#pragma once

#include <string>
#include <vector>

struct PropertyPermissionItem {
    std::wstring id;
    std::wstring name;
    bool editable = true;
};

// 获取当前图纸可配置的属性列表。
std::vector<PropertyPermissionItem> listPropertyPermissions();

// 设置单个属性是否可编辑。
bool setPropertyEditable(const std::wstring& property_id, bool editable, std::wstring* error_message);

// 持久化属性权限（当前为占位实现，后续接入真实存储）。
bool savePropertyPermissions(std::wstring* error_message);
