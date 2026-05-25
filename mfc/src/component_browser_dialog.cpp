#include <Dialog/component_browser_dialog.h>
#include <Resources/mfc_rc.h>
#include <Services/component_browser_backend.h>
#include <acdocman.h>
#include <afxcmn.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

IMPLEMENT_DYNAMIC(ZcBmComponentBrowserDialog, CZcUiDialog)

namespace {
ZcBmComponentBrowserDialog* g_activeComponentBrowserDialog = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void* docKeyFromDocument(AcApDocument* doc) {
    if (doc == nullptr) {
        return nullptr;
    }
    if (doc->database() != nullptr) {
        return doc->database();
    }
    return doc;
}
}  // namespace

// ============================================================
// Impl
//
// 职责划分:
// - Dialog: 控件绑定 + 消息映射
// - Impl  : 交互逻辑 + 视图模型转换 + 菜单行为
//
// 关键状态:
// - m_nodeMap       : TreeItem <-> 业务节点信息映射
// - m_showUnloaded  : 是否显示未加载节点
// ============================================================

class ZcBmComponentBrowserImpl {
   public:
    // 绑定所属对话框，后续所有 UI 访问都通过 m_owner 完成。
    explicit ZcBmComponentBrowserImpl(ZcBmComponentBrowserDialog& owner);

    // 构建完整构件树（不带搜索过滤）。
    void buildTree();
    // 按关键字构建搜索结果树。
    void buildSearchTree(const std::wstring& keyword);
    // 根据当前搜索框内容自动选择全量树或搜索树重建。
    void refreshTree();
    // 同步“布置”按钮是否可用。
    void updatePlaceButton();
    // 切换“显示未加载”开关并刷新树。
    void toggleUnloadedVisibility();
    // 文档切换前保存当前树状态。
    void onBeforeDocumentSwitch();
    // 文档切换后刷新并恢复目标文档状态。
    void onAfterDocumentSwitch();
    // 文档销毁前清理对应缓存桶。
    void onDocumentToBeDestroyed(void* docKey);

    // 处理树选中变化事件。
    void handleSelChanged();
    // 处理双击（Type 节点打开属性）。
    void handleDblClk();
    // 处理右键并弹出上下文菜单。
    void handleRClick();
    // 处理树项自定义绘制（多选高亮）。
    void handleCustomDraw(NMTVCUSTOMDRAW* pCustomDraw, LRESULT* pResult);
    // 处理标签编辑开始，决定是否允许进入编辑态。
    void handleBeginLabelEdit(NMTVDISPINFO* pInfo, LRESULT* pResult);
    // 处理标签编辑结束，执行重命名校验与提交。
    void handleEndLabelEdit(NMTVDISPINFO* pInfo, LRESULT* pResult);
    // 处理键盘快捷键（Ctrl+A/Delete/F2）。
    void handleKeyDown(NMTVKEYDOWN* pKeyDown, LRESULT* pResult);

    // 批量载入模板节点。
    void doContextBatchLoad();
    // 批量删除/卸载模板节点。
    void doContextBatchDelete();
    // 在模板下新建类型。
    void doContextNewType();
    // 编辑当前构件（具体可编辑性由后端判定）。
    void doContextEditComponent();
    // 布置当前类型。
    void doContextPlace();
    // 打开当前类型属性。
    void doContextTypeProperty();
    // 复制当前类型。
    void doContextCopyType();
    // 删除当前类型。
    void doContextDeleteType();
    // 保存当前节点。
    void doContextSave();
    // 卸载当前节点。
    void doContextUnload();
    // 触发当前节点重命名编辑。
    void doContextRename();
    // 删除入口（Type 删除，其他节点卸载）。
    void doContextDelete();

   private:
    struct TreeNodeInfo {
        std::wstring id;
        ComponentNodeLevel level = ComponentNodeLevel::kCategory;
        ComponentLoadState load_state = ComponentLoadState::kNotLoaded;
        bool is_system = false;
    };

    // 插入单个节点并同步 m_nodeMap；返回创建的树项句柄。
    HTREEITEM insertNode(HTREEITEM hParent, const ComponentNode& node);
    // 批量插入子节点（调用 insertNode）。
    void insertChildren(HTREEITEM hParent, const std::vector<ComponentNode>& children);
    // 从指定节点开始递归展开到目标深度。
    void expandToLevel(HTREEITEM hItem, int current_depth, int target_depth);
    // 根据当前选中状态构建并显示上下文菜单。
    void showContextMenu(CPoint pt);
    // 构建类别节点菜单。
    void buildCategoryContextMenu(CMenu& menu, const TreeNodeInfo& info);
    // 构建模板节点菜单。
    void buildTemplateContextMenu(CMenu& menu, const TreeNodeInfo& info);
    // 构建类型节点菜单。
    void buildTypeContextMenu(CMenu& menu, const TreeNodeInfo& info);
    // 构建多选场景下的批量菜单。
    void buildMultiSelectContextMenu(CMenu& menu);
    // 根据树项句柄获取业务节点信息。
    [[nodiscard]] TreeNodeInfo* getNodeInfo(HTREEITEM hItem);
    // 获取当前选中树项列表（多选优先，单选回退）。
    [[nodiscard]] std::vector<HTREEITEM> getSelectedItems() const;
    // 按节点层级过滤并返回选中节点 id 列表。
    [[nodiscard]] std::vector<std::wstring> getSelectedIds(ComponentNodeLevel level) const;
    // 获取当前文档唯一键（优先数据库指针）。
    [[nodiscard]] void* currentDocKey() const;
    // 捕获当前树的展开状态到当前文档桶。
    void captureCurrentExpandedState(bool isSearchMode);
    // 恢复当前文档对应模式的展开状态，返回是否有历史快照。
    [[nodiscard]] bool restoreExpandedStateForCurrentDoc(bool isSearchMode);
    // 删除已关闭文档对应的状态，避免缓存长期增长。
    void pruneDocStateIfNeeded();

    ZcBmComponentBrowserDialog* m_owner = nullptr;
    std::unordered_map<HTREEITEM, TreeNodeInfo> m_nodeMap;
    std::unordered_map<std::wstring, HTREEITEM> m_idToItem;
    struct TreeViewState {
        std::unordered_set<std::wstring> expandedIdsNormal;
        std::unordered_set<std::wstring> expandedIdsSearch;
        bool hasNormalState = false;
        bool hasSearchState = false;
    };
    std::unordered_map<void*, TreeViewState> m_docTreeStates;
    bool m_currentTreeIsSearchMode = false;
    bool m_suspendPreBuildCapture = false;
    bool m_showUnloaded = true;
};

ZcBmComponentBrowserImpl::ZcBmComponentBrowserImpl(ZcBmComponentBrowserDialog& owner) : m_owner(&owner) {}

void ZcBmComponentBrowserImpl::buildTree() {
    // 全量树构建流程:
    // 1) 清空 UI 和缓存映射
    // 2) 拉取完整树模型
    // 3) 递归插入节点
    // 4) 默认展开到一级
    auto& tree = m_owner->m_tree;
    if (!m_suspendPreBuildCapture) {
        captureCurrentExpandedState(m_currentTreeIsSearchMode);
    }
    tree.SetRedraw(FALSE);
    tree.DeleteAllItems();
    m_nodeMap.clear();
    m_idToItem.clear();
    tree.clearInteractionState();

    const auto roots = getComponentTree();
    insertChildren(TVI_ROOT, roots);

    const bool hasState = restoreExpandedStateForCurrentDoc(false);
    if (!hasState) {
        expandToLevel(TVI_ROOT, 0, 1);
    }
    m_currentTreeIsSearchMode = false;

    tree.SetRedraw(TRUE);
    tree.Invalidate();
}

void ZcBmComponentBrowserImpl::buildSearchTree(const std::wstring& keyword) {
    // 搜索树构建与全量构建一致，但数据源换成 searchComponents。
    // 搜索结果默认展开更多层级，便于用户快速定位命中项。
    auto& tree = m_owner->m_tree;
    if (!m_suspendPreBuildCapture) {
        captureCurrentExpandedState(m_currentTreeIsSearchMode);
    }
    tree.SetRedraw(FALSE);
    tree.DeleteAllItems();
    m_nodeMap.clear();
    m_idToItem.clear();
    tree.clearInteractionState();

    const auto results = searchComponents(keyword);
    insertChildren(TVI_ROOT, results);

    const bool hasState = restoreExpandedStateForCurrentDoc(true);
    if (!hasState) {
        expandToLevel(TVI_ROOT, 0, 3);
    }
    m_currentTreeIsSearchMode = true;

    tree.SetRedraw(TRUE);
    tree.Invalidate();
}

void ZcBmComponentBrowserImpl::refreshTree() {
    // 刷新策略:
    // - 搜索框为空 -> 全量树
    // - 搜索框非空 -> 搜索树
    // 同时同步“布置按钮”状态。
    CString searchText;
    m_owner->m_searchEdit.GetWindowText(searchText);
    if (searchText.IsEmpty()) {
        buildTree();
    } else {
        buildSearchTree(std::wstring(searchText.GetString()));
    }
    updatePlaceButton();
}

void ZcBmComponentBrowserImpl::updatePlaceButton() {
    // 仅在“选中 Loaded 的 Type 节点”时允许布置。
    // canPlace 由后端进一步决定是否可执行。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    bool enable = false;
    if (hSel != nullptr) {
        auto* info = getNodeInfo(hSel);
        if (info != nullptr && info->level == ComponentNodeLevel::kType && info->load_state == ComponentLoadState::kLoaded) {
            enable = canPlace(info->id);
        }
    }
    m_owner->m_btnPlace.EnableWindow(enable ? TRUE : FALSE);
}

void ZcBmComponentBrowserImpl::toggleUnloadedVisibility() {
    // 切换过滤开关后直接重建树。
    m_showUnloaded = !m_showUnloaded;
    refreshTree();
}

void ZcBmComponentBrowserImpl::onBeforeDocumentSwitch() { captureCurrentExpandedState(m_currentTreeIsSearchMode); }

void ZcBmComponentBrowserImpl::onAfterDocumentSwitch() {
    pruneDocStateIfNeeded();
    m_suspendPreBuildCapture = true;
    refreshTree();
    m_suspendPreBuildCapture = false;
}

void ZcBmComponentBrowserImpl::onDocumentToBeDestroyed(void* docKey) {
    if (docKey == nullptr) {
        return;
    }
    if (docKey == currentDocKey()) {
        captureCurrentExpandedState(m_currentTreeIsSearchMode);
    }
    m_docTreeStates.erase(docKey);
}

HTREEITEM ZcBmComponentBrowserImpl::insertNode(HTREEITEM hParent, const ComponentNode& node) {
    auto& tree = m_owner->m_tree;

    // 过滤规则: 隐藏未加载开关开启时，仅保留 Category 与 Loaded 节点。
    if (!m_showUnloaded && node.load_state == ComponentLoadState::kNotLoaded && node.level != ComponentNodeLevel::kCategory) {
        return nullptr;
    }

    CString display_name(node.name.c_str());
    if (node.load_state == ComponentLoadState::kNotLoaded && node.level == ComponentNodeLevel::kTemplate) {
        // 当前用后缀提示“未加载”，后续可替换为图标渲染。
        display_name += _T("  (未加载)");
    }

    HTREEITEM hItem = tree.InsertItem(display_name, hParent);
    if (hItem == nullptr) {
        return nullptr;
    }

    TreeNodeInfo info;
    info.id = node.id;
    info.level = node.level;
    info.load_state = node.load_state;
    info.is_system = node.is_system;
    m_nodeMap[hItem] = std::move(info);
    m_idToItem[node.id] = hItem;
    tree.setItemLogicalLevel(hItem, static_cast<int>(node.level));

    if (!node.children.empty()) {
        insertChildren(hItem, node.children);
    }

    if (!m_showUnloaded && node.level == ComponentNodeLevel::kCategory) {
        // 当类别下全部子节点被过滤时，类别自身也不显示。
        if (tree.ItemHasChildren(hItem) == FALSE) {
            tree.DeleteItem(hItem);
            m_nodeMap.erase(hItem);
            m_idToItem.erase(node.id);
            return nullptr;
        }
    }

    return hItem;
}

void ZcBmComponentBrowserImpl::insertChildren(HTREEITEM hParent, const std::vector<ComponentNode>& children) {
    for (const auto& child : children) {
        insertNode(hParent, child);
    }
}

void ZcBmComponentBrowserImpl::expandToLevel(HTREEITEM hItem, int current_depth, int target_depth) {
    // 递归展开到指定层级，避免初始界面信息过载。
    if (current_depth >= target_depth) {
        return;
    }

    auto& tree = m_owner->m_tree;
    HTREEITEM hChild = (hItem == TVI_ROOT) ? tree.GetRootItem() : tree.GetChildItem(hItem);

    while (hChild != nullptr) {
        if (hItem != TVI_ROOT) {
            tree.Expand(hItem, TVE_EXPAND);
        }
        expandToLevel(hChild, current_depth + 1, target_depth);
        hChild = tree.GetNextSiblingItem(hChild);
    }
}

void ZcBmComponentBrowserImpl::handleSelChanged() {
    // 选择变化只做轻量状态同步，复杂选择逻辑在 handleClick 中处理。
    updatePlaceButton();
}

void ZcBmComponentBrowserImpl::handleDblClk() {
    // 双击策略:
    // - Type     -> 打开类型属性
    // - Category/Template -> 交给 TreeCtrl 默认展开/收拢行为
    CPoint pt;
    ::GetCursorPos(&pt);
    CPoint clientPt = pt;
    m_owner->m_tree.ScreenToClient(&clientPt);

    UINT flags = 0;
    HTREEITEM hSel = m_owner->m_tree.HitTest(clientPt, &flags);
    if (hSel == nullptr || (flags & static_cast<UINT>(TVHT_ONITEM)) == 0) {
        hSel = m_owner->m_tree.GetSelectedItem();
    }
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr) {
        return;
    }

    if (info->level == ComponentNodeLevel::kType) {
        std::wstring err;
        showTypeProperty(info->id, &err);
    }
}

void ZcBmComponentBrowserImpl::handleRClick() {
    // 命中项修正由树控件处理，Impl 专注菜单内容构建与分发。
    const CPoint pt = m_owner->m_tree.handleRightClickSelectionAdjust();
    showContextMenu(pt);
}

void ZcBmComponentBrowserImpl::handleCustomDraw(NMTVCUSTOMDRAW* pCustomDraw, LRESULT* pResult) {
    m_owner->m_tree.handleCustomDraw(pCustomDraw, pResult);
}

void ZcBmComponentBrowserImpl::handleBeginLabelEdit(NMTVDISPINFO* pInfo, LRESULT* pResult) {
    // 先默认禁止，再按后端权限放开。
    *pResult = TRUE;
    if (pInfo == nullptr || pInfo->item.hItem == nullptr) {
        return;
    }

    auto* info = getNodeInfo(pInfo->item.hItem);
    if (info == nullptr) {
        return;
    }

    if (!canRename(info->id)) {
        return;
    }

    *pResult = FALSE;
}

void ZcBmComponentBrowserImpl::handleEndLabelEdit(NMTVDISPINFO* pInfo, LRESULT* pResult) {
    // 重命名提交流程:
    // 1) 参数有效性检查
    // 2) validateRename 前置校验
    // 3) renameNode 实际执行
    *pResult = FALSE;
    if (pInfo == nullptr || pInfo->item.pszText == nullptr || pInfo->item.hItem == nullptr) {
        return;
    }

    auto* info = getNodeInfo(pInfo->item.hItem);
    if (info == nullptr) {
        return;
    }

    std::wstring newName(pInfo->item.pszText);
    if (newName.empty()) {
        return;
    }

    auto validation = validateRename(info->id, newName);
    if (!validation.valid) {
        AfxMessageBox(CString(validation.message.c_str()), MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring err;
    if (renameNode(info->id, newName, &err)) {
        *pResult = TRUE;
    } else if (!err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::handleKeyDown(NMTVKEYDOWN* pKeyDown, LRESULT* pResult) {
    // 支持快捷键:
    // - Ctrl+A: 全选（当前实现为根层级）
    // - Delete: 删除当前选中项
    // - F2    : 重命名当前选中项
    m_owner->m_tree.handleKeyDown(pKeyDown, pResult);
    if (pKeyDown == nullptr) {
        return;
    }

    if (pKeyDown->wVKey == VK_DELETE) {
        auto* hSel = m_owner->m_tree.GetSelectedItem();
        if (hSel != nullptr) {
            auto* info = getNodeInfo(hSel);
            if (info != nullptr && canDelete(info->id)) {
                doContextDelete();
            }
        }
    }

    updatePlaceButton();
}

void ZcBmComponentBrowserImpl::showContextMenu(CPoint pt) {
    // 多选显示批量菜单；单选按节点层级切换菜单。
    auto selected = getSelectedItems();
    if (selected.empty()) {
        auto* hSel = m_owner->m_tree.GetSelectedItem();
        if (hSel != nullptr) {
            selected.push_back(hSel);
        }
    }

    if (selected.empty()) {
        return;
    }

    CMenu menu;
    menu.CreatePopupMenu();

    if (selected.size() > 1) {
        buildMultiSelectContextMenu(menu);
    } else {
        auto* info = getNodeInfo(selected.front());
        if (info == nullptr) {
            return;
        }

        switch (info->level) {
            case ComponentNodeLevel::kCategory:
                buildCategoryContextMenu(menu, *info);
                break;
            case ComponentNodeLevel::kTemplate:
                buildTemplateContextMenu(menu, *info);
                break;
            case ComponentNodeLevel::kType:
                buildTypeContextMenu(menu, *info);
                break;
        }
    }

    menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_owner);
}

void ZcBmComponentBrowserImpl::buildCategoryContextMenu(CMenu& menu, const TreeNodeInfo& /*info*/) {
    // 类别节点当前只允许重命名。
    menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
}

void ZcBmComponentBrowserImpl::buildTemplateContextMenu(CMenu& menu, const TreeNodeInfo& info) {
    // 模板菜单会根据是否系统构件、载入状态调整“删除/卸载”等命令。
    if (!info.is_system) {
        menu.AppendMenu(MF_STRING, IDM_CB_BATCH_LOAD, _T("一键载入"));
        menu.AppendMenu(MF_STRING, IDM_CB_NEW_TYPE, _T("新建类型"));
        menu.AppendMenu(MF_STRING, IDM_CB_EDIT_COMPONENT, _T("编辑构件"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, IDM_CB_SAVE, _T("保存"));
        menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
        menu.AppendMenu(MF_SEPARATOR);

        if (info.load_state == ComponentLoadState::kBuiltIn) {
            menu.AppendMenu(MF_STRING, IDM_CB_UNLOAD, _T("卸载"));
        } else {
            menu.AppendMenu(MF_STRING, IDM_CB_DELETE, _T("删除"));
        }
    } else {
        menu.AppendMenu(MF_STRING, IDM_CB_NEW_TYPE, _T("新建类型"));
        menu.AppendMenu(MF_STRING, IDM_CB_EDIT_COMPONENT, _T("编辑构件"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, IDM_CB_SAVE, _T("保存"));
        menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, IDM_CB_UNLOAD, _T("卸载"));
    }
}

void ZcBmComponentBrowserImpl::buildTypeContextMenu(CMenu& menu, const TreeNodeInfo& info) {
    // 类型菜单聚焦于“布置/属性/复制/重命名/删除”。
    if (canPlace(info.id)) {
        menu.AppendMenu(MF_STRING, IDM_CB_PLACE, _T("布置"));
    }
    menu.AppendMenu(MF_STRING, IDM_CB_TYPE_PROPERTY, _T("类型属性"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, IDM_CB_COPY_TYPE, _T("复制"));
    if (canRename(info.id)) {
        menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
    }
    if (canDelete(info.id)) {
        menu.AppendMenu(MF_STRING, IDM_CB_DELETE_TYPE, _T("删除"));
    }
}

void ZcBmComponentBrowserImpl::buildMultiSelectContextMenu(CMenu& menu) {
    // 多选时仅保留可批量执行的命令，避免歧义操作。
    menu.AppendMenu(MF_STRING, IDM_CB_BATCH_LOAD, _T("批量载入"));
    menu.AppendMenu(MF_STRING, IDM_CB_BATCH_DELETE, _T("批量删除/卸载"));
}

ZcBmComponentBrowserImpl::TreeNodeInfo* ZcBmComponentBrowserImpl::getNodeInfo(HTREEITEM hItem) {
    auto it = m_nodeMap.find(hItem);
    return (it != m_nodeMap.end()) ? &it->second : nullptr;
}

std::vector<HTREEITEM> ZcBmComponentBrowserImpl::getSelectedItems() const {
    // 多选语义由自定义树控件维护，这里直接复用其结果。
    return m_owner->m_tree.getEffectiveSelectedItems();
}

std::vector<std::wstring> ZcBmComponentBrowserImpl::getSelectedIds(ComponentNodeLevel level) const {
    // 按层级过滤，供批量命令调用。
    std::vector<std::wstring> ids;
    auto items = getSelectedItems();
    for (auto* hItem : items) {
        auto it = m_nodeMap.find(hItem);
        if (it != m_nodeMap.end() && it->second.level == level) {
            ids.push_back(it->second.id);
        }
    }
    return ids;
}

void* ZcBmComponentBrowserImpl::currentDocKey() const {
    if (acDocManager == nullptr) {
        return nullptr;
    }
    return docKeyFromDocument(acDocManager->curDocument());
}

void ZcBmComponentBrowserImpl::captureCurrentExpandedState(bool isSearchMode) {
    if (m_idToItem.empty()) {
        return;
    }
    void* docKey = currentDocKey();
    if (docKey == nullptr) {
        return;
    }
    auto& treeState = m_docTreeStates[docKey];
    auto& expandedIds = isSearchMode ? treeState.expandedIdsSearch : treeState.expandedIdsNormal;
    expandedIds.clear();

    for (const auto& [componentId, item] : m_idToItem) {
        if (item == nullptr) {
            continue;
        }
        const UINT state = m_owner->m_tree.GetItemState(item, TVIS_EXPANDED);
        if ((state & TVIS_EXPANDED) != 0U) {
            expandedIds.insert(componentId);
        }
    }

    if (isSearchMode) {
        treeState.hasSearchState = true;
    } else {
        treeState.hasNormalState = true;
    }
}

bool ZcBmComponentBrowserImpl::restoreExpandedStateForCurrentDoc(bool isSearchMode) {
    void* docKey = currentDocKey();
    if (docKey == nullptr) {
        return false;
    }

    auto docIt = m_docTreeStates.find(docKey);
    if (docIt == m_docTreeStates.end()) {
        return false;
    }

    const TreeViewState& treeState = docIt->second;
    const bool hasState = isSearchMode ? treeState.hasSearchState : treeState.hasNormalState;
    if (!hasState) {
        return false;
    }

    const auto& expandedIds = isSearchMode ? treeState.expandedIdsSearch : treeState.expandedIdsNormal;
    for (const auto& componentId : expandedIds) {
        auto itemIt = m_idToItem.find(componentId);
        if (itemIt != m_idToItem.end() && itemIt->second != nullptr) {
            m_owner->m_tree.Expand(itemIt->second, TVE_EXPAND);
        }
    }
    return true;
}

void ZcBmComponentBrowserImpl::pruneDocStateIfNeeded() {
    if (m_docTreeStates.empty() || acDocManager == nullptr) {
        return;
    }

    std::unordered_set<void*> aliveDocKeys;
    std::unique_ptr<AcApDocumentIterator> iterator(acDocManager->newAcApDocumentIterator());
    if (iterator != nullptr) {
        for (; !iterator->done(); iterator->step()) {
            aliveDocKeys.insert(docKeyFromDocument(iterator->document()));
        }
    }

    for (auto it = m_docTreeStates.begin(); it != m_docTreeStates.end();) {
        if (aliveDocKeys.find(it->first) == aliveDocKeys.end()) {
            it = m_docTreeStates.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================
// Context menu actions
// ============================================================

void ZcBmComponentBrowserImpl::doContextBatchLoad() {
    // 多选优先，其次回退到当前单选项。
    auto ids = getSelectedIds(ComponentNodeLevel::kTemplate);
    if (ids.empty()) {
        auto* hSel = m_owner->m_tree.GetSelectedItem();
        if (hSel != nullptr) {
            auto* info = getNodeInfo(hSel);
            if (info != nullptr && info->level == ComponentNodeLevel::kTemplate) {
                ids.push_back(info->id);
            }
        }
    }
    if (ids.empty()) {
        return;
    }

    std::wstring err;
    if (!batchLoadComponents(ids, &err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
    refreshTree();
}

void ZcBmComponentBrowserImpl::doContextBatchDelete() {
    // 批量危险操作前二次确认。
    auto ids = getSelectedIds(ComponentNodeLevel::kTemplate);
    if (ids.empty()) {
        return;
    }

    CString msg;
    msg.Format(_T("确定要删除/卸载选中的 %d 个族模板吗？"), static_cast<int>(ids.size()));
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    std::wstring err;
    if (!batchDeleteComponents(ids, &err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
    refreshTree();
}

void ZcBmComponentBrowserImpl::doContextNewType() {
    // 仅模板节点允许“新建类型”，避免在错误层级触发后端命令。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr || info->level != ComponentNodeLevel::kTemplate) {
        return;
    }

    std::wstring newTypeId;
    std::wstring err;
    if (createNewType(info->id, &newTypeId, &err)) {
        refreshTree();
    } else if (!err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextEditComponent() {
    // 编辑入口：Category/Template/Type 都可交由后端决定是否可编辑。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr) {
        return;
    }

    std::wstring err;
    if (!editComponent(info->id, &err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextPlace() {
    // “布置”严格限定在 Type 节点，且可执行性由后端再次校验。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr || info->level != ComponentNodeLevel::kType) {
        return;
    }

    std::wstring err;
    if (!placeComponent(info->id, &err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextTypeProperty() {
    // 打开类型属性面板：仅 Type 节点有效。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr || info->level != ComponentNodeLevel::kType) {
        return;
    }

    std::wstring err;
    if (!showTypeProperty(info->id, &err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextCopyType() {
    // 复制类型成功后刷新树，以展示新生成的类型节点。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr || info->level != ComponentNodeLevel::kType) {
        return;
    }

    std::wstring newTypeId;
    std::wstring err;
    if (copyType(info->id, &newTypeId, &err)) {
        refreshTree();
    } else if (!err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextDeleteType() {
    // 删除类型与“删除模板/卸载模板”是两条独立路径，这里只处理 Type。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr || info->level != ComponentNodeLevel::kType) {
        return;
    }

    std::wstring err;
    if (deleteType(info->id, &err)) {
        refreshTree();
    } else if (!err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextSave() {
    // 保存当前节点到持久层。是否允许保存由后端规则控制。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr) {
        return;
    }

    std::wstring err;
    if (!saveComponent(info->id, &err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextUnload() {
    // 卸载后刷新树，确保“已加载/未加载”状态与过滤结果同步。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr) {
        return;
    }

    std::wstring err;
    if (unloadComponent(info->id, &err)) {
        refreshTree();
    } else if (!err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

void ZcBmComponentBrowserImpl::doContextRename() {
    // 实际重命名校验与提交在 EndLabelEdit 回调中完成，这里只进入编辑态。
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }
    m_owner->m_tree.EditLabel(hSel);
}

void ZcBmComponentBrowserImpl::doContextDelete() {
    // 删除入口统一分流:
    // - Type: deleteType
    // - 其他: unloadComponent
    auto* hSel = m_owner->m_tree.GetSelectedItem();
    if (hSel == nullptr) {
        return;
    }

    auto* info = getNodeInfo(hSel);
    if (info == nullptr) {
        return;
    }

    CString msg;
    msg.Format(_T("确定要删除「%s」吗？"), m_owner->m_tree.GetItemText(hSel).GetString());
    if (AfxMessageBox(msg, MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    std::wstring err;
    bool ok = false;
    if (info->level == ComponentNodeLevel::kType) {
        ok = deleteType(info->id, &err);
    } else {
        ok = unloadComponent(info->id, &err);
    }

    if (ok) {
        refreshTree();
    } else if (!err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
}

// ============================================================
// Dialog
//
// Dialog 层只负责:
// - 将消息路由给 Impl
// - 进行基础控件初始化与控件绑定
// ============================================================

// 构造对话框并创建 Impl；后续事件统一转发给 Impl 处理。
ZcBmComponentBrowserDialog::ZcBmComponentBrowserDialog(CWnd* pParent)
    : CZcUiDialog(IDD_MFC_COMPONENT_BROWSER, pParent), m_parent(pParent), m_impl(std::make_unique<ZcBmComponentBrowserImpl>(*this)) {}

// Impl 采用 RAII 自动释放，这里保持默认析构即可。
ZcBmComponentBrowserDialog::~ZcBmComponentBrowserDialog() {
    if (g_activeComponentBrowserDialog == this) {
        g_activeComponentBrowserDialog = nullptr;
    }
}

void ZcBmComponentBrowserDialog::notifyBeforeDocumentSwitch() {
    if (g_activeComponentBrowserDialog == nullptr || g_activeComponentBrowserDialog->m_impl == nullptr) {
        return;
    }
    if (::IsWindow(g_activeComponentBrowserDialog->GetSafeHwnd()) == FALSE) {
        return;
    }
    g_activeComponentBrowserDialog->m_impl->onBeforeDocumentSwitch();
}

void ZcBmComponentBrowserDialog::notifyAfterDocumentSwitch() {
    if (g_activeComponentBrowserDialog == nullptr || g_activeComponentBrowserDialog->m_impl == nullptr) {
        return;
    }
    if (::IsWindow(g_activeComponentBrowserDialog->GetSafeHwnd()) == FALSE) {
        return;
    }
    g_activeComponentBrowserDialog->m_impl->onAfterDocumentSwitch();
}

void ZcBmComponentBrowserDialog::notifyDocumentToBeDestroyed(void* docKey) {
    if (g_activeComponentBrowserDialog == nullptr || g_activeComponentBrowserDialog->m_impl == nullptr) {
        return;
    }
    g_activeComponentBrowserDialog->m_impl->onDocumentToBeDestroyed(docKey);
}

void ZcBmComponentBrowserDialog::DoDataExchange(CDataExchange* pDX) {
    // 绑定对话框控件到成员变量。
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CB_SEARCH_EDIT, m_searchEdit);
    DDX_Control(pDX, IDC_CB_TREE, m_tree);
    DDX_Control(pDX, IDC_CB_BTN_TOGGLE_VISIBILITY, m_btnToggleVisibility);
    DDX_Control(pDX, IDC_CB_BTN_LOAD, m_btnLoad);
    DDX_Control(pDX, IDC_CB_BTN_PLACE, m_btnPlace);
}

BOOL ZcBmComponentBrowserDialog::OnInitDialog() {
    // 初始化顺序:
    // 1) 设置窗口位置与输入提示
    // 2) 配置 Tree 样式
    // 3) 初始化按钮状态与首次树数据
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    m_searchEdit.SetCueBanner(_T("搜索构件..."));

    m_tree.ModifyStyle(0, TVS_EDITLABELS | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);

    m_btnPlace.EnableWindow(FALSE);
    g_activeComponentBrowserDialog = this;

    m_impl->buildTree();
    m_impl->updatePlaceButton();

    return TRUE;
}

void ZcBmComponentBrowserDialog::onSearchTextChanged() {
    // 搜索框实时过滤；空串回到全量树。
    CString text;
    m_searchEdit.GetWindowText(text);
    if (text.IsEmpty()) {
        m_impl->buildTree();
    } else {
        m_impl->buildSearchTree(std::wstring(text.GetString()));
    }
    m_impl->updatePlaceButton();
}

void ZcBmComponentBrowserDialog::onBnClickedToggleVisibility() {
    // 切换“显示未加载项”开关，内部会触发树重建。
    m_impl->toggleUnloadedVisibility();
}
void ZcBmComponentBrowserDialog::onBnClickedLoad() {
    // 从外部文件载入构件，完成后统一刷新树。
    std::wstring err;
    if (!loadComponentFromFile(&err) && !err.empty()) {
        AfxMessageBox(CString(err.c_str()), MB_OK | MB_ICONERROR);
    }
    m_impl->refreshTree();
}
void ZcBmComponentBrowserDialog::onBnClickedPlace() {
    // 工具栏“布置”复用上下文菜单的同一实现，保证行为一致。
    m_impl->doContextPlace();
}

void ZcBmComponentBrowserDialog::onTreeSelChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    // 选中变化后仅同步界面状态（例如“布置”按钮使能）。
    m_impl->handleSelChanged();
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

void ZcBmComponentBrowserDialog::onTreeDblClk(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    // 双击入口：Type 打开属性，其余节点保持树控件默认行为。
    m_impl->handleDblClk();
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

void ZcBmComponentBrowserDialog::onTreeRClick(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    // 右键入口负责确定菜单上下文与选中项一致性。
    m_impl->handleRClick();
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

void ZcBmComponentBrowserDialog::onTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    // 转发到 Impl 执行多选高亮的自定义绘制。
    m_impl->handleCustomDraw(reinterpret_cast<NMTVCUSTOMDRAW*>(pNMHDR), pResult);
}

void ZcBmComponentBrowserDialog::onTreeBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) {
    // Begin 阶段控制“是否允许编辑标签”。
    m_impl->handleBeginLabelEdit(reinterpret_cast<NMTVDISPINFO*>(pNMHDR), pResult);
}

void ZcBmComponentBrowserDialog::onTreeEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) {
    // End 阶段执行重命名校验与提交。
    m_impl->handleEndLabelEdit(reinterpret_cast<NMTVDISPINFO*>(pNMHDR), pResult);
}

void ZcBmComponentBrowserDialog::onTreeKeyDown(NMHDR* pNMHDR, LRESULT* pResult) {
    // 键盘快捷键入口（Ctrl+A/Delete/F2）。
    m_impl->handleKeyDown(reinterpret_cast<NMTVKEYDOWN*>(pNMHDR), pResult);
}

void ZcBmComponentBrowserDialog::onContextBatchLoad() {
    // 菜单命令转发：批量载入模板。
    m_impl->doContextBatchLoad();
}

void ZcBmComponentBrowserDialog::onContextBatchDelete() {
    // 菜单命令转发：批量删除/卸载模板。
    m_impl->doContextBatchDelete();
}

void ZcBmComponentBrowserDialog::onContextNewType() {
    // 菜单命令转发：在模板下新建类型。
    m_impl->doContextNewType();
}

void ZcBmComponentBrowserDialog::onContextEditComponent() {
    // 菜单命令转发：编辑构件。
    m_impl->doContextEditComponent();
}

void ZcBmComponentBrowserDialog::onContextPlace() {
    // 菜单命令转发：布置类型。
    m_impl->doContextPlace();
}

void ZcBmComponentBrowserDialog::onContextTypeProperty() {
    // 菜单命令转发：打开类型属性。
    m_impl->doContextTypeProperty();
}

void ZcBmComponentBrowserDialog::onContextCopyType() {
    // 菜单命令转发：复制类型。
    m_impl->doContextCopyType();
}

void ZcBmComponentBrowserDialog::onContextDeleteType() {
    // 菜单命令转发：删除类型。
    m_impl->doContextDeleteType();
}

void ZcBmComponentBrowserDialog::onContextSave() {
    // 菜单命令转发：保存当前节点。
    m_impl->doContextSave();
}

void ZcBmComponentBrowserDialog::onContextUnload() {
    // 菜单命令转发：卸载当前节点。
    m_impl->doContextUnload();
}

void ZcBmComponentBrowserDialog::onContextRename() {
    // 菜单命令转发：进入重命名编辑态。
    m_impl->doContextRename();
}

void ZcBmComponentBrowserDialog::onContextDelete() {
    // 菜单命令转发：删除入口（按节点层级分流）。
    m_impl->doContextDelete();
}

BEGIN_MESSAGE_MAP(ZcBmComponentBrowserDialog, CZcUiDialog)
// 搜索输入变化：实时切换全量树/搜索树并同步按钮状态。
ON_EN_CHANGE(IDC_CB_SEARCH_EDIT, &ZcBmComponentBrowserDialog::onSearchTextChanged)

// 顶部功能按钮：切换未加载可见性。
ON_BN_CLICKED(IDC_CB_BTN_TOGGLE_VISIBILITY, &ZcBmComponentBrowserDialog::onBnClickedToggleVisibility)
// 顶部功能按钮：从文件载入构件。
ON_BN_CLICKED(IDC_CB_BTN_LOAD, &ZcBmComponentBrowserDialog::onBnClickedLoad)
// 顶部功能按钮：布置当前类型（与右键菜单“布置”同语义）。
ON_BN_CLICKED(IDC_CB_BTN_PLACE, &ZcBmComponentBrowserDialog::onBnClickedPlace)

// Tree 通知：选中项变化（用于状态同步，例如布置按钮可用性）。
ON_NOTIFY(TVN_SELCHANGED, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeSelChanged)
// Tree 通知：双击（Type 打开属性，其余节点保留默认展开/收拢）。
ON_NOTIFY(NM_DBLCLK, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeDblClk)
// Tree 通知：右键（命中项修正 + 上下文菜单弹出）。
ON_NOTIFY(NM_RCLICK, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeRClick)
// Tree 通知：自定义绘制（附加多选项高亮）。
ON_NOTIFY(NM_CUSTOMDRAW, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeCustomDraw)
// Tree 通知：标签编辑开始（判断是否允许重命名）。
ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeBeginLabelEdit)
// Tree 通知：标签编辑结束（执行校验与重命名提交）。
ON_NOTIFY(TVN_ENDLABELEDIT, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeEndLabelEdit)
// Tree 通知：键盘快捷键（Ctrl+A/Delete/F2）。
ON_NOTIFY(TVN_KEYDOWN, IDC_CB_TREE, &ZcBmComponentBrowserDialog::onTreeKeyDown)

// 菜单命令：批量载入模板。
ON_COMMAND(IDM_CB_BATCH_LOAD, &ZcBmComponentBrowserDialog::onContextBatchLoad)
// 菜单命令：批量删除/卸载模板。
ON_COMMAND(IDM_CB_BATCH_DELETE, &ZcBmComponentBrowserDialog::onContextBatchDelete)
// 菜单命令：新建类型。
ON_COMMAND(IDM_CB_NEW_TYPE, &ZcBmComponentBrowserDialog::onContextNewType)
// 菜单命令：编辑构件。
ON_COMMAND(IDM_CB_EDIT_COMPONENT, &ZcBmComponentBrowserDialog::onContextEditComponent)
// 菜单命令：布置类型。
ON_COMMAND(IDM_CB_PLACE, &ZcBmComponentBrowserDialog::onContextPlace)
// 菜单命令：打开类型属性。
ON_COMMAND(IDM_CB_TYPE_PROPERTY, &ZcBmComponentBrowserDialog::onContextTypeProperty)
// 菜单命令：复制类型。
ON_COMMAND(IDM_CB_COPY_TYPE, &ZcBmComponentBrowserDialog::onContextCopyType)
// 菜单命令：删除类型。
ON_COMMAND(IDM_CB_DELETE_TYPE, &ZcBmComponentBrowserDialog::onContextDeleteType)
// 菜单命令：保存当前节点。
ON_COMMAND(IDM_CB_SAVE, &ZcBmComponentBrowserDialog::onContextSave)
// 菜单命令：卸载当前节点。
ON_COMMAND(IDM_CB_UNLOAD, &ZcBmComponentBrowserDialog::onContextUnload)
// 菜单命令：重命名当前节点。
ON_COMMAND(IDM_CB_RENAME, &ZcBmComponentBrowserDialog::onContextRename)
// 菜单命令：删除入口（Type 删除，模板/类别走卸载路径）。
ON_COMMAND(IDM_CB_DELETE, &ZcBmComponentBrowserDialog::onContextDelete)
END_MESSAGE_MAP()
