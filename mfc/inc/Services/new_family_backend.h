#pragma once

#include <string>
#include <vector>

struct FamilyCategoryOption {
    std::wstring key;          // 稳定键，例如: door / window
    std::wstring display_name; // 展示名，可改
};

struct TemplateOption {
    std::wstring key;       // 稳定键，例如: linear / generic
    std::wstring file_name; // 文件名，可改
};

struct NewFamilyRequest {
    std::wstring category_key;
    std::wstring category_display_name;
    std::wstring template_key;
    std::wstring template_path;
    std::wstring family_name;
};

// 重新加载规则配置（未提供配置时自动使用内置默认规则）。
bool reloadNewFamilyRules(std::wstring* error_message);

// 获取可选类别（供下拉框展示）。
std::vector<FamilyCategoryOption> listFamilyCategories();

// 获取类别显示名（根据 key）。
std::wstring getCategoryDisplayNameByKey(const std::wstring& category_key);

// 根据类别 key 获取默认模板 key。
std::wstring getDefaultTemplateKeyForCategory(const std::wstring& category_key);

// 根据模板 key 获取当前生效模板文件名。
std::wstring getTemplateFileNameByKey(const std::wstring& template_key);

// 根据类别 key 推荐样板文件路径。
std::wstring suggestTemplateFileForCategoryKey(const std::wstring& category_key);

// 根据文件路径反推模板 key；非预定义返回空字符串。
std::wstring resolveTemplateKeyByPath(const std::wstring& template_path);

// 判断路径对应的文件名是否为预定义样板文件。
bool isPredefinedTemplateFile(const std::wstring& template_path);

// 从样板文件路径提取默认族名称（取文件名去扩展名）。
std::wstring suggestFamilyNameFromTemplateFile(const std::wstring& template_path);

// 执行“新建族”后端流程：
// 1) 导入样板文件到新图纸
// 2) 创建族类别
// 3) 创建族
// 4) 创建族类型（默认与族名称一致）
bool executeCreateNewFamily(const NewFamilyRequest& request, std::wstring* error_message);
