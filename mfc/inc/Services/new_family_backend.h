#pragma once

#include <string>
#include <vector>

struct FamilyCategoryOption {
    std::wstring key;          // 稳定键，例如: door / window
    std::wstring display_name; // 展示名，可改
};

struct NewFamilyRequest {
    std::wstring category_key;
    std::wstring template_path;
    std::wstring family_name;
};

// 获取可选类别（供下拉框展示）。
std::vector<FamilyCategoryOption> listFamilyCategories();

// 初始化并缓存“新建族”规则；建议在界面初始化时调用一次。
bool initializeNewFamilyRules(std::wstring* error_message = nullptr);

// 根据类别 key 推荐样板文件路径。
std::wstring suggestTemplateFileForCategoryKey(const std::wstring& category_key);

// 根据文件路径反推模板 key；非预定义返回空字符串。
std::wstring resolveTemplateKeyByPath(const std::wstring& template_path);

// 判断路径对应的文件名是否为预定义样板文件。
bool isPredefinedTemplateFile(const std::wstring& template_path);

// 执行“新建族”后端流程：
// 1) 导入样板文件到新图纸
// 2) 创建族类别
// 3) 创建族
// 4) 创建族类型（默认与族名称一致）
bool executeCreateNewFamily(const NewFamilyRequest& request, std::wstring* error_message);

// 临时文档链路 demo：
// - create db -> read dwg/ztf -> save temp -> open doc -> set title
bool executeCreateNewFamilyTempDemo(const std::wstring& template_path, std::wstring* error_message);
