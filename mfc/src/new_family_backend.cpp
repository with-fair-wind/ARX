#include <Services/new_family_backend.h>
#include <acutads.h>

#include <nlohmann/json.hpp>

#include <array>
#include <cwctype>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (__cplusplus >= 201703L)
#include <filesystem>
#define ARX_FS_NS std::filesystem
#else
#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif
#include <experimental/filesystem>
#define ARX_FS_NS std::experimental::filesystem
#endif

namespace fs = ARX_FS_NS;
using json = nlohmann::json;

namespace {

struct RuleStore {
    std::vector<FamilyCategoryOption> categories;
    std::unordered_map<std::wstring, std::wstring> display_name_by_category_key;
    std::unordered_map<std::wstring, std::wstring> default_template_by_category_key;
    std::unordered_map<std::wstring, std::wstring> file_name_by_template_key;
    std::wstring default_template_key;
    bool loaded = false;
};

std::wstring toLower(std::wstring text) {
    for (wchar_t& ch : text) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    return text;
}

std::wstring trimCopy(const std::wstring& text) {
    const std::wstring ws = L" \t\r\n";
    const std::size_t begin = text.find_first_not_of(ws);
    if (begin == std::wstring::npos) {
        return L"";
    }
    const std::size_t end = text.find_last_not_of(ws);
    return text.substr(begin, end - begin + 1);
}

std::wstring normalizeKey(const std::wstring& key) { return toLower(trimCopy(key)); }

void setError(std::wstring* error_message, const std::wstring& message) {
    if (error_message != nullptr) {
        *error_message = message;
    }
}

RuleStore& rules() {
    static RuleStore store;
    return store;
}

bool fileExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    const DWORD attrs = ::GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring joinPath(const std::wstring& dir, const std::wstring& file_name) {
    if (dir.empty()) {
        return file_name;
    }
    if (dir.back() == L'\\' || dir.back() == L'/') {
        return dir + file_name;
    }
    return dir + L"\\" + file_name;
}

std::wstring getModuleDirectory() {
    std::array<wchar_t, MAX_PATH> module_path = {};
    const DWORD len = ::GetModuleFileNameW(nullptr, module_path.data(), MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L"";
    }
    return fs::path(module_path.data()).parent_path().wstring();
}

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int required = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return L"";
    }
    std::wstring result(static_cast<size_t>(required), L'\0');
    const int converted = ::MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), required);
    if (converted <= 0) {
        return L"";
    }
    return result;
}

void loadDefaultRules(RuleStore& store) {
    store.categories = {
        {L"door", L"门"},        {L"window", L"窗"},      {L"column", L"柱"},     {L"beam", L"梁"},
        {L"stair", L"楼梯"},     {L"elevator", L"电梯"},   {L"escalator", L"电力扶梯"}, {L"furniture", L"家具"},
        {L"sanitary", L"洁具"},  {L"parking", L"车位"},    {L"planting", L"植物"}, {L"rail", L"栏物"},
        {L"legend", L"图例"},    {L"generic", L"常规"},
    };

    store.display_name_by_category_key.clear();
    for (const auto& item : store.categories) {
        store.display_name_by_category_key[normalizeKey(item.key)] = item.display_name;
    }

    store.default_template_key = L"generic";
    store.default_template_by_category_key = {
        {L"door", L"door"},
        {L"window", L"window"},
        {L"column", L"column"},
        {L"beam", L"linear"},
    };

    store.file_name_by_template_key = {
        {L"door", L"门样板.ztt"},
        {L"window", L"窗样板.ztt"},
        {L"column", L"柱样板.ztt"},
        {L"linear", L"线性构件样板.ztt"},
        {L"generic", L"通用样板.ztt"},
    };
}

void overlayRulesFromJson(RuleStore& store, const std::wstring& json_path) {  // NOLINT(readability-function-cognitive-complexity)
    if (json_path.empty() || !fileExists(json_path)) {
        return;
    }

    std::ifstream input(json_path, std::ios::binary);
    if (!input.is_open()) {
        return;
    }

    json root;
    input >> root;

    auto applyCategories = [&store](const json& categories_json) {
        std::vector<FamilyCategoryOption> categories;
        for (const auto& item : categories_json) {
            if (!item.is_object()) {
                continue;
            }
            const std::wstring key = normalizeKey(utf8ToWide(item.value("key", std::string())));
            const std::wstring display_name = trimCopy(utf8ToWide(item.value("display_name", std::string())));
            if (key.empty() || display_name.empty()) {
                continue;
            }
            categories.push_back({key, display_name});
        }
        if (categories.empty()) {
            return;
        }
        store.categories = std::move(categories);
        store.display_name_by_category_key.clear();
        for (const auto& item : store.categories) {
            store.display_name_by_category_key[normalizeKey(item.key)] = item.display_name;
        }
    };

    auto applyTemplates = [&store](const json& templates_json) {
        for (const auto& item : templates_json) {
            if (!item.is_object()) {
                continue;
            }
            const std::wstring key = normalizeKey(utf8ToWide(item.value("key", std::string())));
            const std::wstring file_name = trimCopy(utf8ToWide(item.value("file_name", std::string())));
            if (!key.empty() && !file_name.empty()) {
                store.file_name_by_template_key[key] = file_name;
            }
        }
    };

    auto applyMappings = [&store](const json& mapping_json) {
        for (auto it = mapping_json.begin(); it != mapping_json.end(); ++it) {
            if (!it.value().is_string()) {
                continue;
            }
            const std::wstring key = normalizeKey(utf8ToWide(it.key()));
            const std::wstring value = normalizeKey(utf8ToWide(it.value().get<std::string>()));
            if (key.empty() || value.empty()) {
                continue;
            }
            if (key == L"default") {
                store.default_template_key = value;
            } else {
                store.default_template_by_category_key[key] = value;
            }
        }
    };

    if (root.contains("categories") && root["categories"].is_array()) {
        applyCategories(root["categories"]);
    }
    if (root.contains("templates") && root["templates"].is_array()) {
        applyTemplates(root["templates"]);
    }
    if (root.contains("category_template") && root["category_template"].is_object()) {
        applyMappings(root["category_template"]);
    }
}

void ensureRulesLoaded() {
    RuleStore& store = rules();
    if (store.loaded) {
        return;
    }
    loadDefaultRules(store);
    const std::wstring json_path = joinPath(getModuleDirectory(), L"new_family_rules.json");
    try {
        overlayRulesFromJson(store, json_path);
    } catch (const std::exception&) {
        acutPrintf(_T("\n[新建族] JSON 规则解析失败，回退默认规则。"));
    }
    store.loaded = true;
}

std::wstring makeDefaultFamilyName(const std::wstring& category_name) {
    static std::unordered_map<std::wstring, int> category_counter;
    const std::wstring key = toLower(category_name);
    int& counter = category_counter[key];
    ++counter;
    return category_name + std::to_wstring(counter);
}

bool validateRequest(const NewFamilyRequest& request, std::wstring* error_message) {
    if (request.category_key.empty()) {
        setError(error_message, L"族类别不能为空。");
        return false;
    }
    if (request.template_path.empty()) {
        setError(error_message, L"样板文件不能为空。");
        return false;
    }
    return true;
}

bool importTemplateToNewDrawing(const std::wstring& template_path, std::wstring* error_message) {
    if (!fileExists(template_path)) {
        setError(error_message, L"样板文件不存在: " + template_path);
        return false;
    }
    // TODO: 替换为真实“导入样板文件到新图纸”能力。
    acutPrintf(_T("\n[新建族] 已导入样板文件: %ls"), template_path.c_str());
    return true;
}

bool createFamilyCategory(const std::wstring& category_name, std::wstring* error_message) {
    // TODO: 替换为真实“创建族类别”能力。
    if (category_name.empty()) {
        setError(error_message, L"创建族类别失败：类别名称为空。");
        return false;
    }
    acutPrintf(_T("\n[新建族] 已创建族类别: %ls"), category_name.c_str());
    return true;
}

bool createFamily(const std::wstring& family_name, std::wstring* error_message) {
    // TODO: 替换为真实“创建族”能力。
    if (family_name.empty()) {
        setError(error_message, L"创建族失败：族名称为空。");
        return false;
    }
    acutPrintf(_T("\n[新建族] 已创建族: %ls"), family_name.c_str());
    return true;
}

bool createFamilyType(const std::wstring& family_name, const std::wstring& type_name, std::wstring* error_message) {
    // TODO: 替换为真实“创建族类型”能力。
    if (family_name.empty() || type_name.empty()) {
        setError(error_message, L"创建族类型失败：输入名称为空。");
        return false;
    }
    acutPrintf(_T("\n[新建族] 已创建族类型: %ls (族: %ls)"), type_name.c_str(), family_name.c_str());
    return true;
}

}  // namespace

bool reloadNewFamilyRules(std::wstring* error_message) {
    RuleStore& store = rules();
    loadDefaultRules(store);
    try {
        const std::wstring json_path = joinPath(getModuleDirectory(), L"new_family_rules.json");
        overlayRulesFromJson(store, json_path);
        store.loaded = true;
        if (error_message != nullptr) {
            error_message->clear();
        }
        return true;
    } catch (...) {
        setError(error_message, L"加载新建族规则失败，已回退默认规则。");
        store.loaded = true;
        return false;
    }
}

std::vector<FamilyCategoryOption> listFamilyCategories() {
    ensureRulesLoaded();
    return rules().categories;
}

std::wstring getCategoryDisplayNameByKey(const std::wstring& category_key) {
    ensureRulesLoaded();
    const std::wstring key = normalizeKey(category_key);
    const auto it = rules().display_name_by_category_key.find(key);
    if (it != rules().display_name_by_category_key.end()) {
        return it->second;
    }
    return category_key;
}

std::wstring getDefaultTemplateKeyForCategory(const std::wstring& category_key) {
    ensureRulesLoaded();
    const std::wstring key = normalizeKey(category_key);
    const auto it = rules().default_template_by_category_key.find(key);
    if (it != rules().default_template_by_category_key.end()) {
        return it->second;
    }
    return rules().default_template_key;
}

std::wstring getTemplateFileNameByKey(const std::wstring& template_key) {
    ensureRulesLoaded();
    const std::wstring key = normalizeKey(template_key);
    const auto it = rules().file_name_by_template_key.find(key);
    if (it != rules().file_name_by_template_key.end()) {
        return it->second;
    }
    return L"";
}

std::wstring suggestTemplateFileForCategoryKey(const std::wstring& category_key) {
    return getTemplateFileNameByKey(getDefaultTemplateKeyForCategory(category_key));
}

std::wstring resolveTemplateKeyByPath(const std::wstring& template_path) {
    ensureRulesLoaded();
    const std::wstring file_name = toLower(fs::path(template_path).filename().wstring());
    if (file_name.empty()) {
        return L"";
    }
    for (const auto& entry : rules().file_name_by_template_key) {
        if (toLower(entry.second) == file_name) {
            return entry.first;
        }
    }
    return L"";
}

bool isPredefinedTemplateFile(const std::wstring& template_path) { return !resolveTemplateKeyByPath(template_path).empty(); }

std::wstring suggestFamilyNameFromTemplateFile(const std::wstring& template_path) {
    if (template_path.empty()) {
        return L"";
    }
    return fs::path(template_path).stem().wstring();
}

bool executeCreateNewFamily(const NewFamilyRequest& request, std::wstring* error_message) {
    ensureRulesLoaded();
    if (!validateRequest(request, error_message)) {
        return false;
    }

    const std::wstring category_display_name =
        !request.category_display_name.empty() ? request.category_display_name : getCategoryDisplayNameByKey(request.category_key);
    const std::wstring resolved_family_name =
        request.family_name.empty() ? makeDefaultFamilyName(category_display_name) : request.family_name;

    acutPrintf(_T("\n[新建族] 开始执行后端流程..."));
    if (!importTemplateToNewDrawing(request.template_path, error_message)) {
        return false;
    }
    if (!createFamilyCategory(category_display_name, error_message)) {
        return false;
    }
    if (!createFamily(resolved_family_name, error_message)) {
        return false;
    }
    if (!createFamilyType(resolved_family_name, resolved_family_name, error_message)) {
        return false;
    }
    acutPrintf(_T("\n[新建族] 后端流程执行完成。"));
    return true;
}
