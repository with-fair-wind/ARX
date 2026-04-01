#include <Services/component_browser_backend.h>

#include <acdocman.h>
#include <acedads.h>
#include <aduiFileDialog.h>

#include <algorithm>
#include <cwctype>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_set>
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

// ============================================================
// Stub 实现
//
// 提供一棵静态示例树用于 UI 调试。
// 后续替换为真实的后端逻辑即可。
// ============================================================

namespace {
// 构造 Type 节点的便捷函数，避免样板代码重复。
ComponentNode makeType(const std::wstring& id, const std::wstring& name, ComponentLoadState state, bool is_system) {
    ComponentNode n;
    n.id = id;
    n.name = name;
    n.level = ComponentNodeLevel::kType;
    n.load_state = state;
    n.is_system = is_system;
    return n;
}

// 构造 Template 节点，并接收其下属类型列表。
ComponentNode makeTemplate(const std::wstring& id, const std::wstring& name, ComponentLoadState state, bool is_system, std::vector<ComponentNode> types) {
    ComponentNode n;
    n.id = id;
    n.name = name;
    n.level = ComponentNodeLevel::kTemplate;
    n.load_state = state;
    n.is_system = is_system;
    n.children = std::move(types);
    return n;
}

// 构造 Category 节点，并接收其下属模板列表。
ComponentNode makeCategory(const std::wstring& id, const std::wstring& name, std::vector<ComponentNode> templates) {
    ComponentNode n;
    n.id = id;
    n.name = name;
    n.level = ComponentNodeLevel::kCategory;
    n.load_state = ComponentLoadState::kLoaded;
    n.children = std::move(templates);
    return n;
}

// 不区分大小写的子串匹配（宽字符）。
bool containsIgnoreCase(const std::wstring& haystack, const std::wstring& needle) {
    if (needle.empty()) {
        return true;
    }
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                          [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); });
    return it != haystack.end();
}

std::wstring toLower(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return text;
}

bool equalsIgnoreCase(const std::wstring& left, const std::wstring& right) { return toLower(left) == toLower(right); }

std::wstring sanitizeFileName(std::wstring file_name) {
    static const std::wstring kIllegalChars = L"\\/:*?\"<>|";
    for (wchar_t& ch : file_name) {
        if (kIllegalChars.find(ch) != std::wstring::npos) {
            ch = L'_';
        }
    }
    return file_name;
}

std::wstring makeTemplateFileName(const std::wstring& template_id) { return template_id + L".ztf"; }

bool fileNameEqualsIgnoreCase(const fs::path& path, const std::wstring& file_name) { return equalsIgnoreCase(path.filename().wstring(), file_name); }

bool findFileByNameInDirectory(const fs::path& dir, const std::wstring& file_name, fs::path* matched_path) {
    if (matched_path == nullptr || !fs::exists(dir) || !fs::is_directory(dir)) {
        return false;
    }

    const fs::path direct_hit = dir / file_name;
    if (fs::exists(direct_hit) && fs::is_regular_file(direct_hit)) {
        *matched_path = direct_hit;
        return true;
    }

    try {
        for (fs::recursive_directory_iterator it(dir), end; it != end; ++it) {
            const auto& entry = *it;
            if (!entry.is_regular_file()) {
                continue;
            }
            const fs::path& current = entry.path();
            if (fileNameEqualsIgnoreCase(current, file_name)) {
                *matched_path = current;
                return true;
            }
        }
    } catch (const fs::filesystem_error&) {
        return false;
    }

    return false;
}

AcApDocument* findOpenedTemplateDocument(const std::wstring& template_file_name) {
    std::unique_ptr<AcApDocumentIterator> iterator(acDocManager->newAcApDocumentIterator());
    if (iterator == nullptr) {
        return nullptr;
    }

    for (; !iterator->done(); iterator->step()) {
        AcApDocument* doc = iterator->document();
        if (doc == nullptr) {
            continue;
        }

        const ZTCHAR* file_name = doc->fileName();
        if (file_name != nullptr && *file_name != 0) {
            const fs::path path(file_name);
            if (fileNameEqualsIgnoreCase(path, template_file_name)) {
                return doc;
            }
        }
    }

    return nullptr;
}

std::vector<fs::path> collectSearchRoots() {
    std::vector<fs::path> roots;
    std::unordered_set<std::wstring> seen;

    auto add_root = [&roots, &seen](const fs::path& root) {
        if (root.empty()) {
            return;
        }
        const std::wstring normalized = toLower(root.wstring());
        if (seen.insert(normalized).second) {
            roots.push_back(root);
        }
    };

    const DWORD drives = ::GetLogicalDrives();
    for (wchar_t ch = L'A'; ch <= L'Z'; ++ch) {
        const DWORD bit = (static_cast<DWORD>(1) << static_cast<DWORD>(ch - L'A'));
        if ((drives & bit) == 0) {
            continue;
        }
        const std::wstring root = std::wstring(1, ch) + L":\\";
        if (::GetDriveTypeW(root.c_str()) == DRIVE_FIXED) {
            add_root(fs::path(root));
        }
    }
    return roots;
}

bool findFileGlobally(const std::wstring& file_name, fs::path* matched_path) {
    if (matched_path == nullptr) {
        return false;
    }

    constexpr std::size_t kMaxVisitedEntries = 500000;
    std::size_t visited_entries = 0;
    const auto roots = collectSearchRoots();

    for (const fs::path& root : roots) {
        if (findFileByNameInDirectory(root, file_name, matched_path)) {
            return true;
        }

        // 防止全盘搜索耗时过长导致界面假死。
        visited_entries += 100000;
        if (visited_entries >= kMaxVisitedEntries) {
            break;
        }
    }
    return false;
}

bool showSaveAsDialog(const fs::path& default_path, fs::path* selected_path, std::wstring* error_message) {
    if (selected_path == nullptr) {
        return false;
    }
    const CString default_name(default_path.wstring().c_str());
    CWnd* main_wnd = CWnd::FromHandle(adsw_acadMainWnd());
    CAdUiFileDialog dlg(FALSE, _T("ztf"), default_name, OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, _T("Template (*.ztf)|*.ztf||"), main_wnd);

    if (dlg.DoModal() != IDOK) {
        if (error_message != nullptr) {
            error_message->clear();
        }
        return false;
    }

    CString target_path = dlg.GetPathName();
    if (target_path.IsEmpty()) {
        if (error_message != nullptr) {
            *error_message = L"未选择有效保存路径。";
        }
        return false;
    }

    CString lower_path = target_path;
    lower_path.MakeLower();
    if (lower_path.GetLength() < 4 || lower_path.Right(4) != _T(".ztf")) {
        target_path += _T(".ztf");
    }

    *selected_path = fs::path(target_path.GetString());
    return true;
}

bool saveOpenedDocumentAs(AcApDocument* target_doc, const fs::path& selected_path, std::wstring* error_message) {
    if (target_doc == nullptr || target_doc->database() == nullptr) {
        if (error_message != nullptr) {
            *error_message = L"无法获取目标构件文档。";
        }
        return false;
    }

    AcApDocument* current_doc = acDocManager->curDocument();
    bool should_unlock = false;
    if (target_doc != current_doc) {
        const Acad::ErrorStatus lock_status = acDocManager->lockDocument(target_doc, AcAp::kWrite);
        if (lock_status != Acad::eOk) {
            if (error_message != nullptr) {
                *error_message = L"锁定目标构件文档失败，错误码: " + std::to_wstring(static_cast<int>(lock_status));
            }
            return false;
        }
        should_unlock = true;
    }

    const Acad::ErrorStatus status = target_doc->database()->saveAs(selected_path.c_str(), true, AcDb::kDHL_CURRENT);
    if (should_unlock) {
        acDocManager->unlockDocument(target_doc);
    }
    if (status != Acad::eOk) {
        if (error_message != nullptr) {
            *error_message = L"另存为失败，错误码: " + std::to_wstring(static_cast<int>(status));
        }
        return false;
    }

    return true;
}

bool copyLocalFileAs(const fs::path& source_path, const fs::path& target_path, std::wstring* error_message) {
    std::error_code ec;
    fs::copy_file(source_path, target_path, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        if (error_message != nullptr) {
            const std::string msg = ec.message();
            *error_message = L"文件另存为失败: " + std::wstring(msg.begin(), msg.end());
        }
        return false;
    }
    return true;
}

// 递归过滤节点:
// - Type: 直接按名称匹配
// - Category/Template: 若自身命中则保留全部子树；否则仅保留命中的后代路径
// matched 作为输出参数，告知调用方该节点（或其后代）是否命中。
ComponentNode filterNode(const ComponentNode& node, const std::wstring& keyword, bool& matched) {
    matched = false;

    if (node.level == ComponentNodeLevel::kType) {
        matched = containsIgnoreCase(node.name, keyword);
        return node;
    }

    ComponentNode result = node;
    result.children.clear();

    bool selfMatch = containsIgnoreCase(node.name, keyword);

    for (const auto& child : node.children) {
        bool childMatched = false;
        auto filtered = filterNode(child, keyword, childMatched);
        if (childMatched || selfMatch) {
            result.children.push_back(std::move(filtered));
        }
    }

    matched = selfMatch || !result.children.empty();
    return result;
}

}  // namespace

std::vector<ComponentNode> getComponentTree() {
    // 示例数据仅用于 UI 联调，真实项目中应由文档/数据库/文件系统驱动。
    std::vector<ComponentNode> tree;

    tree.push_back(makeCategory(L"cat_window", L"窗", {
        makeTemplate(L"tpl_single_casement", L"单扇平开窗", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_single_900x1200", L"900*1200", ComponentLoadState::kLoaded, false),
            makeType(L"typ_single_1200x1500", L"1200*1500", ComponentLoadState::kLoaded, false),
            makeType(L"typ_single_1000x1200", L"1000*1200", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_double_casement", L"双扇平开窗", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_double_1600x1500", L"1600*1500", ComponentLoadState::kLoaded, false),
            makeType(L"typ_double_1800x2000", L"1800*2000", ComponentLoadState::kLoaded, false),
            makeType(L"typ_double_2000x2200", L"2000*2200", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_fixed_window", L"固定窗", ComponentLoadState::kLoaded, true, {
            makeType(L"typ_fixed_1200x1200", L"1200*1200", ComponentLoadState::kLoaded, true),
            makeType(L"typ_fixed_1800x1500", L"1800*1500", ComponentLoadState::kLoaded, true),
        }),
        makeTemplate(L"tpl_sliding", L"悬拉窗", ComponentLoadState::kNotLoaded, false, {}),
        makeTemplate(L"tpl_corner", L"转角窗", ComponentLoadState::kNotLoaded, false, {}),
    }));

    tree.push_back(makeCategory(L"cat_wall", L"墙", {
        makeTemplate(L"tpl_basic_wall", L"基本墙", ComponentLoadState::kLoaded, true, {
            makeType(L"typ_wall_200", L"200mm", ComponentLoadState::kLoaded, true),
            makeType(L"typ_wall_240", L"240mm", ComponentLoadState::kLoaded, true),
            makeType(L"typ_wall_300", L"300mm", ComponentLoadState::kLoaded, true),
        }),
        makeTemplate(L"tpl_curtain_wall_unit", L"单元式幕墙墙体", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_cw_unit_150", L"单元板块-150", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_cw_unit_180", L"单元板块-180", ComponentLoadState::kNotLoaded, false),
        }),
    }));

    tree.push_back(makeCategory(L"cat_door", L"门", {
        makeTemplate(L"tpl_single_door", L"单扇门", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_door_single_900x2100", L"900*2100", ComponentLoadState::kLoaded, false),
            makeType(L"typ_door_single_1000x2100", L"1000*2100", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_double_door", L"双开门", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_door_double_1500x2100", L"1500*2100", ComponentLoadState::kLoaded, false),
            makeType(L"typ_door_double_1800x2400", L"1800*2400", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_fire_door", L"防火门", ComponentLoadState::kNotLoaded, true, {
            makeType(L"typ_fire_door_a", L"甲级防火门", ComponentLoadState::kNotLoaded, true),
            makeType(L"typ_fire_door_b", L"乙级防火门", ComponentLoadState::kNotLoaded, true),
        }),
    }));

    tree.push_back(makeCategory(L"cat_stair", L"楼梯", {
        makeTemplate(L"tpl_stair_straight", L"直跑楼梯", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_stair_straight_3000", L"层高3000", ComponentLoadState::kLoaded, false),
            makeType(L"typ_stair_straight_3300", L"层高3300", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_stair_u_shape", L"双跑楼梯", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_stair_u_3000", L"双跑-3000", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_stair_u_3600", L"双跑-3600", ComponentLoadState::kNotLoaded, false),
        }),
    }));

    tree.push_back(makeCategory(L"cat_roof", L"屋顶", {
        makeTemplate(L"tpl_flat_roof", L"平屋顶", ComponentLoadState::kLoaded, true, {
            makeType(L"typ_flat_roof_150", L"保温层150", ComponentLoadState::kLoaded, true),
            makeType(L"typ_flat_roof_200", L"保温层200", ComponentLoadState::kLoaded, true),
        }),
        makeTemplate(L"tpl_slope_roof", L"坡屋顶", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_slope_roof_15", L"坡度15deg", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_slope_roof_30", L"坡度30deg", ComponentLoadState::kNotLoaded, false),
        }),
    }));

    tree.push_back(makeCategory(L"cat_column", L"柱", {
        makeTemplate(L"tpl_rect_column", L"矩形柱", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_rect_col_500x500", L"500*500", ComponentLoadState::kLoaded, false),
            makeType(L"typ_rect_col_600x600", L"600*600", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_round_column", L"圆柱", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_round_col_d500", L"D500", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_round_col_d700", L"D700", ComponentLoadState::kNotLoaded, false),
        }),
    }));

    tree.push_back(makeCategory(L"cat_curtain_wall", L"幕墙", {
        makeTemplate(L"tpl_curtain_wall_stick", L"框架式幕墙", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_cw_stick_150", L"竖梃150", ComponentLoadState::kLoaded, false),
            makeType(L"typ_cw_stick_180", L"竖梃180", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_curtain_wall_unitized", L"单元式幕墙", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_cw_unitized_1200", L"板块宽1200", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_cw_unitized_1500", L"板块宽1500", ComponentLoadState::kNotLoaded, false),
        }),
    }));

    tree.push_back(makeCategory(L"cat_furniture", L"家具", {
        makeTemplate(L"tpl_desk", L"办公桌", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_desk_1200", L"1200*600", ComponentLoadState::kLoaded, false),
            makeType(L"typ_desk_1400", L"1400*700", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_chair", L"办公椅", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_chair_standard", L"标准办公椅", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_chair_ergonomic", L"人体工学椅", ComponentLoadState::kNotLoaded, false),
        }),
        makeTemplate(L"tpl_storage", L"储物柜", ComponentLoadState::kNotLoaded, false, {}),
    }));

    tree.push_back(makeCategory(L"cat_equipment", L"设备", {
        makeTemplate(L"tpl_ahu", L"空调机组", ComponentLoadState::kLoaded, true, {
            makeType(L"typ_ahu_5000", L"5000m3/h", ComponentLoadState::kLoaded, true),
            makeType(L"typ_ahu_8000", L"8000m3/h", ComponentLoadState::kLoaded, true),
        }),
        makeTemplate(L"tpl_fan_coil", L"风机盘管", ComponentLoadState::kLoaded, false, {
            makeType(L"typ_fc_2p", L"2P", ComponentLoadState::kLoaded, false),
            makeType(L"typ_fc_3p", L"3P", ComponentLoadState::kLoaded, false),
        }),
        makeTemplate(L"tpl_pump", L"循环泵", ComponentLoadState::kNotLoaded, false, {
            makeType(L"typ_pump_25_160", L"25-160", ComponentLoadState::kNotLoaded, false),
            makeType(L"typ_pump_40_200", L"40-200", ComponentLoadState::kNotLoaded, false),
        }),
    }));

    return tree;
}

std::vector<ComponentNode> searchComponents(const std::wstring& keyword) {
    // 当前 stub 策略:
    // 1) 先拿完整树
    // 2) 递归剪裁为“包含命中链路”的子树
    // 3) 返回给 UI 直接渲染
    auto tree = getComponentTree();
    std::vector<ComponentNode> result;

    for (const auto& root : tree) {
        bool matched = false;
        auto filtered = filterNode(root, keyword, matched);
        if (matched) {
            result.push_back(std::move(filtered));
        }
    }

    return result;
}

bool batchLoadComponents(const std::vector<std::wstring>& /*template_ids*/, std::wstring* error_message) {
    // TODO: 实现批量载入逻辑
    // 建议在真实实现中返回“部分成功”的细节，以便 UI 精确提示。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 批量载入尚未实现";
    }
    return false;
}

bool batchDeleteComponents(const std::vector<std::wstring>& /*template_ids*/, std::wstring* error_message) {
    // TODO: 实现批量删除逻辑
    // 可按节点属性区分 delete/unload，并在失败时返回首个错误原因。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 批量删除尚未实现";
    }
    return false;
}

bool loadComponentFromFile(std::wstring* error_message) {
    // TODO: 弹出文件浏览框，选择构件文件并加载
    // 建议在后端完成路径校验、格式校验、重复导入策略处理。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 从文件载入尚未实现";
    }
    return false;
}

bool placeComponent(const std::wstring& /*type_id*/, std::wstring* error_message) {
    // TODO: 实例化类型到绘图区
    // 若处于命令上下文，需要注意文档锁与命令重入约束。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 布置构件尚未实现";
    }
    return false;
}

bool createNewType(const std::wstring& /*template_id*/, std::wstring* /*new_type_id*/, std::wstring* error_message) {
    // TODO: 在族模板下新建类型
    // new_type_id 可返回实际创建 ID，供 UI 精确定位并高亮。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 新建类型尚未实现";
    }
    return false;
}

bool editComponent(const std::wstring& /*template_id*/, std::wstring* error_message) {
    // TODO: 进入构件编辑器
    // 可能涉及文档切换/打开模板文件，建议统一在后端管控上下文。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 编辑构件尚未实现";
    }
    return false;
}

bool showTypeProperty(const std::wstring& /*type_id*/, std::wstring* error_message) {
    // TODO: 打开类型属性面板
    // 可在后端内部复用“属性面板”服务，避免 UI 层直接依赖具体实现。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 类型属性尚未实现";
    }
    return false;
}

bool copyType(const std::wstring& /*type_id*/, std::wstring* /*new_type_id*/, std::wstring* error_message) {
    // TODO: 复制类型
    // 复制时建议遵循“名称后缀递增”的统一规则。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 复制类型尚未实现";
    }
    return false;
}

bool deleteType(const std::wstring& /*type_id*/, std::wstring* error_message) {
    // TODO: 删除类型
    // 若类型被实例引用，后端应返回清晰失败原因并可附修复建议。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 删除类型尚未实现";
    }
    return false;
}

bool saveComponent(const std::wstring& template_id, std::wstring* error_message) {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    const std::wstring safe_template_id = sanitizeFileName(template_id);
    if (safe_template_id.empty()) {
        if (error_message != nullptr) {
            *error_message = L"无效的构件标识，无法保存。";
        }
        return false;
    }

    const std::wstring template_file_name = makeTemplateFileName(safe_template_id);

    // 1) 先在当前会话已打开文档中查找 id.ztf
    AcApDocument* opened_doc = findOpenedTemplateDocument(template_file_name);
    if (opened_doc != nullptr) {
        fs::path default_path = fs::path(template_file_name);
        const ZTCHAR* file_name = opened_doc->fileName();
        if (file_name != nullptr && *file_name != 0) {
            default_path = fs::path(file_name);
        }

        fs::path selected_path;
        if (!showSaveAsDialog(default_path, &selected_path, error_message)) {
            return false;
        }
        return saveOpenedDocumentAs(opened_doc, selected_path, error_message);
    }

    // 2) 未打开则全局搜索本地 id.ztf 文件
    fs::path found_local_file;
    if (!findFileGlobally(template_file_name, &found_local_file)) {
        if (error_message != nullptr) {
            error_message->clear();
        }
        return false;
    }

    // 3) 命中文件后执行另存为（文件级复制）
    fs::path selected_path;
    if (!showSaveAsDialog(found_local_file, &selected_path, error_message)) {
        return false;
    }
    return copyLocalFileAs(found_local_file, selected_path, error_message);
}

bool unloadComponent(const std::wstring& /*template_id*/, std::wstring* error_message) {
    // TODO: 卸载构件
    // 卸载前可校验是否存在正在使用的实例或未保存修改。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 卸载构件尚未实现";
    }
    return false;
}

RenameValidation validateRename(const std::wstring& /*node_id*/, const std::wstring& new_name) {
    // 这里仅保留基础格式校验。
    // 真正的“重名冲突校验”建议结合 node_id 在后端数据源中执行。
    RenameValidation result;

    if (new_name.empty()) {
        result.valid = false;
        result.message = L"名称不能为空";
        return result;
    }

    const std::wstring illegal = L"#&?";
    for (wchar_t ch : new_name) {
        if (illegal.find(ch) != std::wstring::npos) {
            result.valid = false;
            result.message = L"请输入不包含# & ? 及不可打印字符的文本";
            return result;
        }
    }

    return result;
}

bool renameNode(const std::wstring& /*node_id*/, const std::wstring& /*new_name*/, std::wstring* error_message) {
    // TODO: 实际重命名逻辑
    // 建议: 先 validateRename -> 再事务性写入 -> 最后发布变更事件给 UI。
    if (error_message != nullptr) {
        *error_message = L"[Stub] 重命名尚未实现";
    }
    return false;
}

// 权限判断在 stub 中统一放开。
// 真实实现建议按“节点类型 + 系统属性 + 文档状态 + 用户权限”综合判断。
bool canPlace(const std::wstring& /*node_id*/) { return true; }
bool canEdit(const std::wstring& /*node_id*/) { return true; }
bool canDelete(const std::wstring& /*node_id*/) { return true; }
bool canUnload(const std::wstring& /*node_id*/) { return true; }
bool canSave(const std::wstring& /*node_id*/) { return true; }
bool canRename(const std::wstring& /*node_id*/) { return true; }
