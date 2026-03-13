#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ============================================================
// 数据模型
//
// 说明:
// - UI 端通过 ComponentNode 表示树形结构中的任意节点。
// - 节点层级固定为三层: Category -> Template -> Type。
// - 后端可以直接返回完整树，也可以在搜索接口中返回剪裁后的子树。
// ============================================================

enum class ComponentNodeLevel : std::uint8_t {
    kCategory,  // 一级：类别 (Category)
    kTemplate,  // 二级：族模板 (Template)
    kType,      // 三级：类型 (Type)
};

enum class ComponentLoadState : std::uint8_t {
    kLoaded,
    kNotLoaded,
    kBuiltIn,
};

struct ComponentNode {
    // 节点唯一标识。建议使用稳定 ID（数据库主键 / 路径 / GUID），避免仅使用展示名。
    std::wstring id;
    // 节点展示名称（树上可见文本，不应承担唯一性约束）。
    std::wstring name;
    // 节点层级，用于决定菜单、双击行为、可执行动作等。
    ComponentNodeLevel level = ComponentNodeLevel::kCategory;
    // 载入状态，UI 会据此决定是否隐藏“未加载”节点和按钮可用性。
    ComponentLoadState load_state = ComponentLoadState::kNotLoaded;
    // 是否系统构件。系统构件与普通构件在菜单命令（删除/卸载）上有差异。
    bool is_system = false;
    // 父节点 ID（可选，便于后端回溯路径；UI 当前主要依赖 children 关系）。
    std::wstring parent_id;
    // 子节点列表。Category 的子节点是 Template，Template 的子节点是 Type。
    std::vector<ComponentNode> children;
};

struct RenameValidation {
    // 是否通过校验。
    bool valid = true;
    // 校验失败原因（用于 UI 提示）。
    std::wstring message;
};

// ============================================================
// 后端接口
//
// 所有函数为纯接口声明，由后端实现。
// Dialog 层仅通过这些接口与业务逻辑交互。
// ============================================================

/// 获取完整构件树（工程树形式，包含全部 Category/Template/Type）。
/// 返回值用于主面板初次加载和“清空搜索”后的恢复。
std::vector<ComponentNode> getComponentTree();

/// 模糊搜索构件。
/// 建议返回“保留父链”的剪裁子树，保证 UI 仍可展示层级上下文。
std::vector<ComponentNode> searchComponents(const std::wstring& keyword);

/// 一键载入：将选中的未加载族模板节点从预设路径加载到当前文档。
/// @param template_ids 族模板 ID 集合（允许多选批量）。
/// @param error_message 失败原因（可空）。
/// @return true 表示调用成功且业务完成；false 表示失败。
bool batchLoadComponents(const std::vector<std::wstring>& template_ids, std::wstring* error_message);

/// 批量删除/卸载族模板。
/// 对系统构件通常执行“卸载”，对非系统构件可执行“删除”。
bool batchDeleteComponents(const std::vector<std::wstring>& template_ids, std::wstring* error_message);

/// 载入本地构件文件到当前文档（由后端决定是否弹出文件选择框）。
bool loadComponentFromFile(std::wstring* error_message);

/// 布置构件：将选中 Type 实例化到绘图区。
bool placeComponent(const std::wstring& type_id, std::wstring* error_message);

/// 在构件节点下新建类型
bool createNewType(const std::wstring& template_id, std::wstring* new_type_id, std::wstring* error_message);

/// 编辑构件（进入构件编辑器环境）
bool editComponent(const std::wstring& template_id, std::wstring* error_message);

/// 查看/编辑类型属性
bool showTypeProperty(const std::wstring& type_id, std::wstring* error_message);

/// 复制类型
bool copyType(const std::wstring& type_id, std::wstring* new_type_id, std::wstring* error_message);

/// 删除类型（从当前文档的族模板下删除）
bool deleteType(const std::wstring& type_id, std::wstring* error_message);

/// 保存构件到本地文件
bool saveComponent(const std::wstring& template_id, std::wstring* error_message);

/// 卸载构件（内置构件卸载，非系统构件删除）
bool unloadComponent(const std::wstring& template_id, std::wstring* error_message);

/// 重命名前置校验。
/// 建议在此集中处理: 空字符串、非法字符、重名规则、只读节点限制等。
RenameValidation validateRename(const std::wstring& node_id, const std::wstring& new_name);

/// 执行重命名（在 validateRename 通过后调用）。
bool renameNode(const std::wstring& node_id, const std::wstring& new_name, std::wstring* error_message);

/// 判断节点是否可以执行某操作。
/// UI 会据此启用/禁用按钮和右键菜单项。
bool canPlace(const std::wstring& node_id);
bool canEdit(const std::wstring& node_id);
bool canDelete(const std::wstring& node_id);
bool canUnload(const std::wstring& node_id);
bool canSave(const std::wstring& node_id);
bool canRename(const std::wstring& node_id);
