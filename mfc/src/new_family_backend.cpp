#include <Services/new_family_backend.h>
#include <Utils/path_utils.h>
#include <Utils/string_utils.h>
#include <acestext.h>
#include <acdocman.h>
#include <acutads.h>
#include <windows.h>

#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../3rdparty/tinyxml/tinyxml.h"

namespace {

// 规则运行态：每条规则就是“类别名 <-> 模板文件”的一对一配置。
struct TemplateRule {
    std::wstring category_name;
    std::wstring template_file;
};

struct RuleSet {
    std::vector<TemplateRule> rules;
    std::wstring rulesBaseDirectory;
    std::wstring loadError;

    static RuleSet& active() {
        static RuleSet rules;
        return rules;
    }
};

// 统一小写化（用于 key 归一化）。
std::wstring toLower(std::wstring text) { return StringUtils::toLowerCopy(std::move(text)); }

// 去除首尾空白。
std::wstring trimCopy(const std::wstring& text) { return StringUtils::trimCopy(text); }

// 文本键统一归一化：trim + lowercase。
std::wstring normalizeKey(const std::wstring& key) { return toLower(trimCopy(key)); }

// 模板文件名归一化（用于反向索引查找）。
std::wstring normalizeTemplateFileNameKey(const std::wstring& pathOrName) { return toLower(trimCopy(PathUtils::filename(pathOrName))); }

// 可选错误输出辅助函数。
void setError(std::wstring* errorMessage, const std::wstring& message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::wstring formatErrorStatus(Zcad::ErrorStatus status) {
    std::wstring message = L"错误码: " + std::to_wstring(static_cast<int>(status));
    const ZTCHAR* statusText = acadErrorStatusText(status);
    if (statusText != nullptr && *statusText != 0) {
        message += L"（";
        message += statusText;
        message += L"）";
    }
    return message;
}


// 获取宿主进程模块目录（兜底）。
std::wstring getModuleDirectory() {
    std::array<wchar_t, MAX_PATH> modulePath = {};
    // nullptr 表示取宿主进程模块（仅作兜底）。
    const DWORD len = ::GetModuleFileNameW(nullptr, modulePath.data(), MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L"";
    }
    const std::wstring fullPath(modulePath.data());
    const std::size_t pos = fullPath.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return fullPath.substr(0, pos);
}

// 获取当前插件模块句柄（优先用于定位规则文件目录）。
HMODULE getCurrentModuleHandle() {
    // 使用函数内静态锚点，避免全局变量暴露。
    static const int moduleAddressAnchor = 0;
    MEMORY_BASIC_INFORMATION info = {};
    if (::VirtualQuery(static_cast<const void*>(&moduleAddressAnchor), &info, sizeof(info)) == 0) {
        return nullptr;
    }
    return static_cast<HMODULE>(info.AllocationBase);
}

// 通过模块句柄获取目录。
std::wstring getModuleDirectoryByHandle(HMODULE moduleHandle) {
    if (moduleHandle == nullptr) {
        return L"";
    }
    // 指定模块句柄，拿到插件自身路径。
    std::array<wchar_t, MAX_PATH> modulePath = {};
    const DWORD len = ::GetModuleFileNameW(moduleHandle, modulePath.data(), MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return L"";
    }
    const std::wstring fullPath(modulePath.data());
    const std::size_t pos = fullPath.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return L"";
    }
    return fullPath.substr(0, pos);
}

// UTF-16 -> UTF-8（TinyXML 路径/文本需要 UTF-8）。
std::string wideToUtf8(const wchar_t* text) {
    if (text == nullptr || *text == L'\0') {
        return "";
    }
    // 1) 先探测 UTF-8 缓冲区长度。
    const int sourceLen = static_cast<int>(std::wcslen(text));
    const int required = ::WideCharToMultiByte(CP_UTF8, 0, text, sourceLen, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return "";
    }
    // 2) 分配目标缓冲并执行转换。
    std::string out(static_cast<size_t>(required), '\0');
    const int converted = ::WideCharToMultiByte(CP_UTF8, 0, text, sourceLen, &out.front(), required, nullptr, nullptr);
    if (converted <= 0) {
        return "";
    }
    return out;
}

// UTF-8 -> UTF-16（TinyXML 读取属性后统一转宽字符串）。
std::wstring utf8ToWide(const char* text) {
    if (text == nullptr || *text == '\0') {
        return L"";
    }
    // 1) 先计算 UTF-16 长度。
    const int len = static_cast<int>(std::strlen(text));
    const int required = ::MultiByteToWideChar(CP_UTF8, 0, text, len, nullptr, 0);
    if (required <= 0) {
        return L"";
    }
    // 2) 分配目标缓冲并执行转换。
    std::wstring out(static_cast<size_t>(required), L'\0');
    const int converted = ::MultiByteToWideChar(CP_UTF8, 0, text, len, &out.front(), required);
    if (converted <= 0) {
        return L"";
    }
    return out;
}

// 清空规则状态。严格模式下，加载前先 reset，避免脏状态残留。
void resetRules(RuleSet& rules) { rules = RuleSet{}; }

const TemplateRule* findRuleByCategoryName(const RuleSet& rules, const std::wstring& categoryName) {
    const std::wstring normalizedName = normalizeKey(categoryName);
    for (const auto& rule : rules.rules) {
        if (normalizeKey(rule.category_name) == normalizedName) {
            return &rule;
        }
    }
    return nullptr;
}

// 统一规范化规则，避免大小写/空白差异导致匹配失败。
void normalizeRules(RuleSet& rules) {
    std::vector<TemplateRule> normalizedRules;
    normalizedRules.reserve(rules.rules.size());

    for (const auto& rule : rules.rules) {
        const std::wstring categoryName = trimCopy(rule.category_name);
        const std::wstring templateFile = trimCopy(rule.template_file);
        if (categoryName.empty() || templateFile.empty()) {
            continue;
        }
        normalizedRules.push_back({categoryName, templateFile});
    }
    rules.rules = std::move(normalizedRules);
}

// 将“模板文件名/相对路径”解析为规则目录下的候选绝对路径。
// 如果本身是绝对路径，原样返回。
std::wstring resolveConfiguredTemplatePath(const RuleSet& rules, const std::wstring& fileNameOrPath) {
    std::wstring candidate = trimCopy(fileNameOrPath);
    // 绝对路径或无规则目录时，不做拼接，原样返回。
    if (candidate.empty() || PathUtils::isAbsolute(candidate) || rules.rulesBaseDirectory.empty()) {
        return candidate;
    }
    // 相对输入统一按“规则目录 + 文件名/相对路径”处理。
    return PathUtils::join(rules.rulesBaseDirectory, candidate);
}

// 解析并校验 XML 规则（严格模式）。
// 精简版格式：
// <new_family_rules>
//   <rules>
//     <rule category_name="门" template_file="门样板.ztf" />
//   </rules>
// </new_family_rules>
bool parseRulesFromXml(RuleSet& rules, const std::wstring& xmlPath, std::wstring* errorMessage) {  // NOLINT(readability-function-cognitive-complexity)
    // 1) 规则文件存在性检查。
    if (xmlPath.empty() || !PathUtils::isFile(xmlPath)) {
        setError(errorMessage, L"规则文件不存在: " + xmlPath);
        return false;
    }

    // 2) 加载 XML 文档（TinyXML 使用 UTF-8 路径）。
    TiXmlDocument doc;
    const std::string utf8Path = wideToUtf8(xmlPath.c_str());
    if (!doc.LoadFile(utf8Path.c_str(), TIXML_ENCODING_UTF8)) {
        setError(errorMessage, L"规则文件解析失败: " + xmlPath);
        return false;
    }

    // 3) 校验根节点名称。
    TiXmlElement* root = doc.RootElement();
    if (root == nullptr || std::strcmp(root->Value(), "new_family_rules") != 0) {
        setError(errorMessage, L"规则文件根节点错误，期望 new_family_rules。");
        return false;
    }

    // 4) 解析 rules：每条规则只包含类别名和模板文件。
    TiXmlElement* rulesNode = root->FirstChildElement("rules");
    if (rulesNode == nullptr) {
        setError(errorMessage, L"规则文件缺少 rules 节点。");
        return false;
    }

    std::vector<TemplateRule> parsedRules;
    parsedRules.reserve(16);

    for (TiXmlElement* item = rulesNode->FirstChildElement("rule"); item != nullptr; item = item->NextSiblingElement("rule")) {
        const std::wstring categoryName = trimCopy(utf8ToWide(item->Attribute("category_name")));
        const std::wstring templateFile = trimCopy(utf8ToWide(item->Attribute("template_file")));
        if (categoryName.empty() || templateFile.empty()) {
            continue;
        }
        parsedRules.push_back({categoryName, templateFile});
    }
    if (parsedRules.empty()) {
        setError(errorMessage, L"规则文件无有效规则项，至少需要一条 category_name + template_file。");
        return false;
    }

    // 5) 提交解析结果并做规范化。
    rules.rules = std::move(parsedRules);
    normalizeRules(rules);
    if (rules.rules.empty()) {
        setError(errorMessage, L"规则文件无有效规则项，至少需要一条 category_name + template_file。");
        return false;
    }

    return true;
}

// 每次调用都重新从 XML 读取规则（不做内存缓存）。
// 严格模式：XML 任一校验失败，当前调用直接不可用。
bool ensureRulesLoaded(RuleSet& rules, std::wstring* errorMessage = nullptr) {
    // 1) 先重置，确保每次调用都基于最新 XML 重建。
    resetRules(rules);
    // 2) 优先使用插件目录，宿主目录仅兜底。
    const std::wstring pluginDir = getModuleDirectoryByHandle(getCurrentModuleHandle());
    rules.rulesBaseDirectory = pluginDir.empty() ? getModuleDirectory() : pluginDir;
    const std::wstring xmlPath = PathUtils::join(rules.rulesBaseDirectory, L"FileTemplateRules.xml");
    // 3) 严格解析 XML；失败则回传可读错误并保持未加载状态。
    if (!parseRulesFromXml(rules, xmlPath, &rules.loadError)) {
        setError(errorMessage, rules.loadError);
        return false;
    }
    return true;
}

// 判断类别名是否存在于规则集。
bool containsCategoryName(const RuleSet& rules, const std::wstring& categoryName) { return findRuleByCategoryName(rules, categoryName) != nullptr; }

// 根据类别名返回显示名；找不到则回退原输入。
std::wstring categoryDisplayNameByName(const RuleSet& rules, const std::wstring& categoryName) {
    if (const TemplateRule* rule = findRuleByCategoryName(rules, categoryName)) {
        return rule->category_name;
    }
    return categoryName;
}

// 根据类别名获取模板文件名；若类别未命中，回退第一条规则。
std::wstring templateFileForCategoryName(const RuleSet& rules, const std::wstring& categoryName) {
    if (const TemplateRule* rule = findRuleByCategoryName(rules, categoryName)) {
        return rule->template_file;
    }
    if (rules.rules.empty()) {
        return L"";
    }
    return rules.rules.front().template_file;
}

// 根据路径（或文件名）反查预定义模板“类别名”。
std::wstring resolveCategoryNameByPathInternal(const RuleSet& rules, const std::wstring& templatePath) {
    const std::wstring inputPathKey = PathUtils::normalizeForCompare(templatePath);
    if (inputPathKey.empty()) {
        return L"";
    }
    for (const auto& rule : rules.rules) {
        const std::wstring configuredFullPath = resolveConfiguredTemplatePath(rules, rule.template_file);
        if (PathUtils::normalizeForCompare(configuredFullPath) == inputPathKey) {
            return rule.category_name;
        }
    }
    return L"";
}

// 执行前模板路径归一化：
// 1) 绝对路径且存在 -> 直接用
// 2) 仅文件名且能匹配预定义模板 -> 规则目录下拼接后验证
// 3) 作为相对路径在规则目录尝试
// 4) 都失败则返回原输入，交给后续统一报错
std::wstring resolveTemplatePathForExecution(const RuleSet& rules, const std::wstring& templatePath) {
    std::wstring input = trimCopy(templatePath);
    if (input.empty()) {
        return L"";
    }
    if (PathUtils::isFile(input) || PathUtils::isAbsolute(input)) {
        return input;
    }

    const std::wstring categoryName = resolveCategoryNameByPathInternal(rules, input);
    if (!categoryName.empty()) {
        const std::wstring configuredPath = resolveConfiguredTemplatePath(rules, templateFileForCategoryName(rules, categoryName));
        if (PathUtils::isFile(configuredPath)) {
            return configuredPath;
        }
    }

    // 兼容“预定义模板只显示文件名”的输入场景。
    const std::wstring fileNameKey = normalizeTemplateFileNameKey(input);
    for (const auto& rule : rules.rules) {
        if (normalizeTemplateFileNameKey(rule.template_file) != fileNameKey) {
            continue;
        }
        const std::wstring configuredPath = resolveConfiguredTemplatePath(rules, rule.template_file);
        if (PathUtils::isFile(configuredPath)) {
            return configuredPath;
        }
    }

    std::wstring relativePath = resolveConfiguredTemplatePath(rules, input);
    if (PathUtils::isFile(relativePath)) {
        return relativePath;
    }
    return input;
}

// 自动生成族名（类别名 + 递增计数）。
std::wstring makeDefaultFamilyName(const std::wstring& categoryName) {
    static std::unordered_map<std::wstring, int> categoryCounter;
    const std::wstring key = toLower(categoryName);
    int& counter = categoryCounter[key];
    ++counter;
    return categoryName + std::to_wstring(counter);
}

// 业务流程服务：只处理“新建族执行流程”。
// 规则加载/解析仍由上面的规则函数负责（函数式分层）。
class NewFamilyService {
   public:
    // 主流程：
    // - 规则可用性校验
    // - 请求归一化与验证
    // - 新建数据库并导入模板（.ztf 视为 dwg）
    // - 伪函数修改族类别/族/族类型名称
    // - 激活该临时文档并标记为未保存
    static bool executeCreate(const NewFamilyRequest& request, std::wstring* errorMessage) {
        RuleSet& activeRules = RuleSet::active();
        if (activeRules.rules.empty()) {
            if (!initializeNewFamilyRules(errorMessage)) {
                return false;
            }
        }
        if (activeRules.rules.empty()) {
            return false;
        }

        NewFamilyRequest resolvedRequest = request;
        resolvedRequest.template_path = resolveTemplatePathForExecution(activeRules, request.template_path);

        if (!validateRequest(activeRules, resolvedRequest, errorMessage)) {
            return false;
        }

        const std::wstring categoryDisplayName = categoryDisplayNameByName(activeRules, resolvedRequest.category_name);
        const std::wstring resolvedFamilyName = resolvedRequest.family_name.empty() ? makeDefaultFamilyName(categoryDisplayName) : resolvedRequest.family_name;

        acutPrintf(_T("\n[新建族] 开始执行后端流程..."));
        if (!buildAndActivateTemporaryFamilyDocument(resolvedRequest.template_path, categoryDisplayName, resolvedFamilyName, resolvedFamilyName, errorMessage)) {
            return false;
        }
        acutPrintf(_T("\n[新建族] 后端流程执行完成。"));
        return true;
    }

   private:
    // 输入参数校验（严格模式补充了类别存在性检查）。
    static bool validateRequest(const RuleSet& rules, const NewFamilyRequest& request, std::wstring* errorMessage) {
        if (request.category_name.empty()) {
            setError(errorMessage, L"族类别不能为空。");
            return false;
        }
        if (!containsCategoryName(rules, request.category_name)) {
            setError(errorMessage, L"族类别在规则中不存在: " + request.category_name);
            return false;
        }
        if (request.template_path.empty()) {
            setError(errorMessage, L"样板文件不能为空。");
            return false;
        }
        return true;
    }

    static bool createUnnamedTargetDocument(AcApDocument** outDoc, std::wstring* errorMessage) {
        if (outDoc == nullptr) {
            setError(errorMessage, L"输出文档指针为空。");
            return false;
        }
        *outDoc = nullptr;

        const Zcad::ErrorStatus status = acDocManager->appContextNewDocument(nullptr);
        if (status != Zcad::eOk) {
            setError(errorMessage, L"创建未命名文档失败，错误码: " + std::to_wstring(static_cast<int>(status)));
            return false;
        }

        AcApDocument* doc = acDocManager->curDocument();
        if (doc == nullptr || doc->database() == nullptr) {
            setError(errorMessage, L"已创建文档，但获取目标文档数据库失败。");
            return false;
        }
        *outDoc = doc;
        return true;
    }

    static bool importSideDbToTargetDb(AcDbDatabase* targetDb, AcDbDatabase* sideDb, std::wstring* errorMessage) {
        if (targetDb == nullptr || sideDb == nullptr) {
            setError(errorMessage, L"导入失败：目标数据库或源数据库为空。");
            return false;
        }

        // 先尝试保留源数据库（最安全），失败后再尝试非保留模式做兼容。
        Zcad::ErrorStatus status = targetDb->insert(AcGeMatrix3d::kIdentity, sideDb, true);
        if (status == Zcad::eOk) {
            return true;
        }
        const Zcad::ErrorStatus fallbackStatus = targetDb->insert(AcGeMatrix3d::kIdentity, sideDb, false);
        if (fallbackStatus == Zcad::eOk) {
            return true;
        }

        setError(errorMessage, L"导入模板数据库失败。首次尝试[" + formatErrorStatus(status) + L"]；回退尝试[" + formatErrorStatus(fallbackStatus) + L"]");
        return false;
    }

    // 伪函数：后续替换为真实“类别/族/类型”改名逻辑。
    static bool applyFamilyMetadataPseudo(AcDbDatabase* workingDb, const std::wstring& categoryName, const std::wstring& familyName, const std::wstring& typeName, std::wstring* errorMessage) {
        if (workingDb == nullptr) {
            setError(errorMessage, L"临时数据库无效。");
            return false;
        }
        if (categoryName.empty() || familyName.empty() || typeName.empty()) {
            setError(errorMessage, L"类别/族/族类型名称不能为空。");
            return false;
        }
        // TODO: 接入真实 API，对数据库中的类别、族、类型执行命名写入。
        acutPrintf(_T("\n[新建族] 伪实现：设置族类别=%ls, 族=%ls, 族类型=%ls"), categoryName.c_str(), familyName.c_str(), typeName.c_str());
        return true;
    }

    // 聚合后的单入口：
    // 1) appContextNewDocument 创建未命名目标文档
    // 2) sideDb.readDwgFile 读取模板
    // 3) insert/wblock 路径导入目标文档数据库
    // 4) 伪函数改类别/族/类型名并保持未保存状态
    static bool buildAndActivateTemporaryFamilyDocument(const std::wstring& templatePath, const std::wstring& categoryName, const std::wstring& familyName, const std::wstring& typeName,
                                                        std::wstring* errorMessage) {
        if (!PathUtils::isFile(templatePath)) {
            setError(errorMessage, L"样板文件不存在: " + templatePath);
            return false;
        }
        if (acDocManager == nullptr) {
            setError(errorMessage, L"文档管理器不可用。");
            return false;
        }

        AcApDocument* targetDoc = nullptr;
        if (!createUnnamedTargetDocument(&targetDoc, errorMessage)) {
            return false;
        }
        AcDbDatabase* targetDb = targetDoc->database();
        if (targetDb == nullptr) {
            setError(errorMessage, L"目标文档数据库不可用。");
            return false;
        }

        std::unique_ptr<AcDbDatabase> sideDb = std::make_unique<AcDbDatabase>(false, true);
        const Zcad::ErrorStatus readStatus = sideDb->readDwgFile(templatePath.c_str(), AcDbDatabase::kForReadAndReadShare, false);
        if (readStatus != Zcad::eOk) {
            setError(errorMessage, L"读取模板失败，" + formatErrorStatus(readStatus));
            return false;
        }

        if (!importSideDbToTargetDb(targetDb, sideDb.get(), errorMessage)) {
            return false;
        }

        if (!applyFamilyMetadataPseudo(targetDb, categoryName, familyName, typeName, errorMessage)) {
            return false;
        }
        const std::wstring maskedTitle = familyName.empty() ? L"未命名族" : familyName;
        targetDoc->setDocTitle(maskedTitle.c_str());
        acdbSetDbmod(targetDb, 1);
        acutPrintf(_T("\n[新建族] 已创建未命名文档并导入模板数据库。"));
        return true;
    }

   public:
    static bool executeTempDemo(const std::wstring& templatePath, std::wstring* errorMessage) {
        return buildAndActivateTemporaryFamilyDocument(templatePath, L"DemoCategory", L"DemoFamily", L"DemoType", errorMessage);
    }
};

}  // namespace

// ===== 对外接口（保持与头文件约定一致）=====

// 返回可选类别列表。严格模式下规则加载失败则返回空列表。
std::vector<std::wstring> listFamilyCategories() {
    const RuleSet& rules = RuleSet::active();
    if (rules.rules.empty()) {
        return {};
    }
    std::vector<std::wstring> categories;
    categories.reserve(rules.rules.size());
    for (const auto& rule : rules.rules) {
        categories.push_back(rule.category_name);
    }
    return categories;
}

// 根据类别返回推荐模板路径（规则目录下）。
std::wstring suggestTemplateFileForCategoryName(const std::wstring& category_name) {
    const RuleSet& rules = RuleSet::active();
    return resolveConfiguredTemplatePath(rules, templateFileForCategoryName(rules, category_name));
}

// 反查预定义模板对应类别名（不抛错，找不到返回空字符串）。
std::wstring resolveCategoryNameByTemplatePath(const std::wstring& template_path) {
    const RuleSet& rules = RuleSet::active();
    return resolveCategoryNameByPathInternal(rules, template_path);
}

// 便捷判断：是否预定义模板文件。
bool isPredefinedTemplateFile(const std::wstring& template_path) { return !resolveCategoryNameByTemplatePath(template_path).empty(); }

// 业务流程入口。
bool initializeNewFamilyRules(std::wstring* error_message) { return ensureRulesLoaded(RuleSet::active(), error_message); }

bool executeCreateNewFamily(const NewFamilyRequest& request, std::wstring* error_message) { return NewFamilyService::executeCreate(request, error_message); }

bool executeCreateNewFamilyTempDemo(const std::wstring& template_path, std::wstring* error_message) {
    return NewFamilyService::executeTempDemo(template_path, error_message);
}
