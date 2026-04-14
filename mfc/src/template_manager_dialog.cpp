#include <Dialog/template_manager_dialog.h>

#include <Control/template_tree_ctrl.h>
#include <Model/template_tree_model.h>
#include <Resources/mfc_rc.h>
#include <Services/template_manager_backend.h>
#include <ViewModel/template_tree_view_model.h>

#include <memory>
#include <unordered_map>
#include <vector>

IMPLEMENT_DYNAMIC(ZcBmTemplateManagerDlg, CZcUiDialog)

class ZcBmTemplateManagerDlg::Impl {
   public:
    explicit Impl(CWnd* parent)
        : m_parentWnd(parent),
          m_backend(createTemplateManagerBackend()),
          m_treeModel(m_backend.m_repository, m_backend.m_commandService, m_backend.m_permissionService),
          m_treeViewModel(m_treeModel) {}

    HTREEITEM insertViewNode(ZcBmTemplateTreeCtrl& treeCtrl, HTREEITEM parent, const TemplateViewNode& viewNode) {
        const TemplateNode* node = m_treeModel.findNode(viewNode.m_nodeId);
        if (node == nullptr) {
            return nullptr;
        }
        CString text(node->m_name.c_str());
        if (node->m_type == TemplateElementType::eTemplate && node->m_loadState == TemplateLoadState::kNotLoaded) {
            text += _T("  (未加载)");
        }

        HTREEITEM item = treeCtrl.InsertItem(text, parent);
        if (item == nullptr) {
            return nullptr;
        }

        treeCtrl.SetItemData(item, static_cast<DWORD_PTR>(viewNode.m_nodeId));
        treeCtrl.bindItemType(item, node->m_type);
        m_itemByNodeId[viewNode.m_nodeId] = item;
        for (std::vector<TemplateViewNode>::const_iterator it = viewNode.m_children.begin(); it != viewNode.m_children.end(); ++it) {
            insertViewNode(treeCtrl, item, *it);
        }
        return item;
    }

    void rebuildTree(ZcBmTemplateTreeCtrl& treeCtrl) {
        treeCtrl.SetRedraw(FALSE);
        treeCtrl.DeleteAllItems();
        treeCtrl.clearInteractionState();
        m_itemByNodeId.clear();

        const std::vector<TemplateViewNode> roots = m_treeViewModel.buildViewTree();
        for (std::vector<TemplateViewNode>::const_iterator it = roots.begin(); it != roots.end(); ++it) {
            insertViewNode(treeCtrl, TVI_ROOT, *it);
        }
        treeCtrl.SetRedraw(TRUE);
        treeCtrl.Invalidate();
    }

    void showErrorIfNeeded(const std::wstring& errorMessage) const {
        if (!errorMessage.empty()) {
            AfxMessageBox(CString(errorMessage.c_str()), MB_OK | MB_ICONWARNING);
        }
    }

    void syncSelectionFromTree(ZcBmTemplateTreeCtrl& treeCtrl) {
        std::vector<TemplateNodeId> selectedIds;
        const std::vector<HTREEITEM> selectedItems = treeCtrl.getEffectiveSelectedItems();
        for (std::vector<HTREEITEM>::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
            const TemplateNodeId nodeId = treeCtrl.itemNodeId(*it);
            if (nodeId != kInvalidTemplateNodeId) {
                selectedIds.push_back(nodeId);
            }
        }

        TemplateNodeId focusedId = kInvalidTemplateNodeId;
        HTREEITEM focusedItem = treeCtrl.GetSelectedItem();
        if (focusedItem != nullptr) {
            focusedId = treeCtrl.itemNodeId(focusedItem);
        }
        m_treeViewModel.setSelection(std::move(selectedIds), focusedId);
    }

    void updatePlaceButton(CButton& placeButton) { placeButton.EnableWindow(m_treeViewModel.canPlaceFocused() ? TRUE : FALSE); }

    void refreshTree(ZcBmTemplateTreeCtrl& treeCtrl, CButton& placeButton) {
        std::wstring errorMessage;
        if (!m_treeViewModel.refresh(&errorMessage)) {
            showErrorIfNeeded(errorMessage);
        }
        rebuildTree(treeCtrl);
        updatePlaceButton(placeButton);
    }

    void showContextMenu(CWnd* owner, ZcBmTemplateTreeCtrl& treeCtrl, CPoint screenPoint) {
        CMenu menu;
        menu.CreatePopupMenu();

        const std::vector<TemplateNodeId> selectedTemplates = m_treeViewModel.selectedIdsByType(TemplateElementType::eTemplate);
        if (selectedTemplates.size() > 1U) {
            menu.AppendMenu(MF_STRING, IDM_CB_BATCH_LOAD, _T("批量载入"));
            menu.AppendMenu(MF_STRING, IDM_CB_BATCH_DELETE, _T("批量删除/卸载"));
            menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, owner);
            return;
        }

        const TemplateNode* focusedNode = m_treeViewModel.focusedNode();
        if (focusedNode == nullptr) {
            return;
        }

        if (focusedNode->m_type == TemplateElementType::eTopLevelTemplateCategory || focusedNode->m_type == TemplateElementType::eSubTemplateCategory) {
            menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
        } else if (focusedNode->m_type == TemplateElementType::eTemplate) {
            menu.AppendMenu(MF_STRING, IDM_CB_BATCH_LOAD, _T("一键载入"));
            menu.AppendMenu(MF_STRING, IDM_CB_NEW_TYPE, _T("新建类型"));
            menu.AppendMenu(MF_STRING, IDM_CB_EDIT_COMPONENT, _T("编辑模板"));
            menu.AppendMenu(MF_SEPARATOR);
            menu.AppendMenu(MF_STRING, IDM_CB_SAVE, _T("保存"));
            menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
            menu.AppendMenu(MF_SEPARATOR);
            menu.AppendMenu(MF_STRING, IDM_CB_DELETE, _T("删除/卸载"));
        } else if (focusedNode->m_type == TemplateElementType::eTemplateType) {
            if (m_treeViewModel.canPlaceFocused()) {
                menu.AppendMenu(MF_STRING, IDM_CB_PLACE, _T("布置"));
            }
            menu.AppendMenu(MF_STRING, IDM_CB_TYPE_PROPERTY, _T("类型属性"));
            menu.AppendMenu(MF_SEPARATOR);
            menu.AppendMenu(MF_STRING, IDM_CB_COPY_TYPE, _T("复制"));
            menu.AppendMenu(MF_STRING, IDM_CB_RENAME, _T("重命名"));
            menu.AppendMenu(MF_STRING, IDM_CB_DELETE_TYPE, _T("删除"));
        }

        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, owner);
    }

    CWnd* m_parentWnd = nullptr;
    CEdit m_searchEdit;
    ZcBmTemplateTreeCtrl m_treeCtrl;
    CButton m_buttonToggleVisibility;
    CButton m_buttonLoad;
    CButton m_buttonPlace;
    TemplateManagerBackendBundle m_backend;
    TemplateTreeModel m_treeModel;
    TemplateTreeViewModel m_treeViewModel;
    std::unordered_map<TemplateNodeId, HTREEITEM> m_itemByNodeId;
};

ZcBmTemplateManagerDlg::ZcBmTemplateManagerDlg(CWnd* pParent)
    : CZcUiDialog(IDD_MFC_COMPONENT_BROWSER, pParent), m_impl(new Impl(pParent)) {}

ZcBmTemplateManagerDlg::~ZcBmTemplateManagerDlg() = default;

void ZcBmTemplateManagerDlg::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CB_SEARCH_EDIT, m_impl->m_searchEdit);
    DDX_Control(pDX, IDC_CB_TREE, m_impl->m_treeCtrl);
    DDX_Control(pDX, IDC_CB_BTN_TOGGLE_VISIBILITY, m_impl->m_buttonToggleVisibility);
    DDX_Control(pDX, IDC_CB_BTN_LOAD, m_impl->m_buttonLoad);
    DDX_Control(pDX, IDC_CB_BTN_PLACE, m_impl->m_buttonPlace);
}

BOOL ZcBmTemplateManagerDlg::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_impl->m_parentWnd != nullptr) {
        CenterWindow(m_impl->m_parentWnd);
    }

    m_impl->m_searchEdit.SetCueBanner(_T("搜索族模板..."));
    m_impl->m_treeCtrl.ModifyStyle(0, TVS_EDITLABELS | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS);
    m_impl->m_buttonPlace.EnableWindow(FALSE);

    m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    return TRUE;
}

void ZcBmTemplateManagerDlg::onSearchTextChanged() {
    CString text;
    m_impl->m_searchEdit.GetWindowText(text);
    m_impl->m_treeViewModel.setKeyword(std::wstring(text.GetString()));
    m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
}

void ZcBmTemplateManagerDlg::onBnClickedToggleVisibility() {
    m_impl->m_treeViewModel.setShowUnloaded(!m_impl->m_treeViewModel.showUnloaded());
    m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
}

void ZcBmTemplateManagerDlg::onBnClickedLoad() {
    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doBatchLoad(&err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onBnClickedPlace() {
    std::wstring err;
    m_impl->m_treeViewModel.doPlace(&err);
    m_impl->showErrorIfNeeded(err);
}

void ZcBmTemplateManagerDlg::onTreeSelChanged(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    m_impl->syncSelectionFromTree(m_impl->m_treeCtrl);
    m_impl->updatePlaceButton(m_impl->m_buttonPlace);
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

void ZcBmTemplateManagerDlg::onTreeDblClk(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    m_impl->syncSelectionFromTree(m_impl->m_treeCtrl);
    const TemplateNode* focusedNode = m_impl->m_treeViewModel.focusedNode();
    if (focusedNode != nullptr && focusedNode->m_type == TemplateElementType::eTemplateType) {
        std::wstring err;
        m_impl->m_treeViewModel.doShowTypeProperty(&err);
        m_impl->showErrorIfNeeded(err);
    }
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

void ZcBmTemplateManagerDlg::onTreeRClick(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    const CPoint screenPoint = m_impl->m_treeCtrl.handleRightClickSelectionAdjust();
    m_impl->syncSelectionFromTree(m_impl->m_treeCtrl);
    m_impl->showContextMenu(this, m_impl->m_treeCtrl, screenPoint);
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

void ZcBmTemplateManagerDlg::onTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    m_impl->m_treeCtrl.handleCustomDraw(reinterpret_cast<NMTVCUSTOMDRAW*>(pNMHDR), pResult);
}

void ZcBmTemplateManagerDlg::onTreeBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) {
    if (pResult == nullptr) {
        return;
    }
    *pResult = TRUE;
    auto* info = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
    if (info == nullptr || info->item.hItem == nullptr) {
        return;
    }

    std::vector<TemplateNodeId> selected;
    selected.push_back(m_impl->m_treeCtrl.itemNodeId(info->item.hItem));
    m_impl->m_treeViewModel.setSelection(selected, m_impl->m_treeCtrl.itemNodeId(info->item.hItem));
    if (m_impl->m_treeViewModel.canRenameFocused()) {
        *pResult = FALSE;
    }
}

void ZcBmTemplateManagerDlg::onTreeEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) {
    if (pResult == nullptr) {
        return;
    }
    *pResult = FALSE;

    auto* info = reinterpret_cast<NMTVDISPINFO*>(pNMHDR);
    if (info == nullptr || info->item.hItem == nullptr || info->item.pszText == nullptr) {
        return;
    }

    const TemplateNodeId nodeId = m_impl->m_treeCtrl.itemNodeId(info->item.hItem);
    m_impl->m_treeViewModel.setSelection({nodeId}, nodeId);

    const std::wstring newName(info->item.pszText);
    RenameValidation validation = m_impl->m_treeViewModel.validateRenameFocused(newName);
    if (!validation.valid) {
        m_impl->showErrorIfNeeded(validation.message);
        return;
    }

    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doRename(newName, &err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        *pResult = TRUE;
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onTreeKeyDown(NMHDR* pNMHDR, LRESULT* pResult) {
    auto* keyDown = reinterpret_cast<NMTVKEYDOWN*>(pNMHDR);
    m_impl->m_treeCtrl.handleKeyDown(keyDown, pResult);
    if (keyDown == nullptr) {
        return;
    }

    m_impl->syncSelectionFromTree(m_impl->m_treeCtrl);
    if (keyDown->wVKey == VK_DELETE) {
        onContextDelete();
    }
    m_impl->updatePlaceButton(m_impl->m_buttonPlace);
}

void ZcBmTemplateManagerDlg::onContextBatchLoad() {
    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doBatchLoad(&err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onContextBatchDelete() {
    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doBatchDeleteOrUnload(&err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onContextNewType() {
    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doCreateType(&err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onContextEditTemplate() {
    std::wstring err;
    m_impl->m_treeViewModel.doEditTemplate(&err);
    m_impl->showErrorIfNeeded(err);
}

void ZcBmTemplateManagerDlg::onContextPlace() { onBnClickedPlace(); }

void ZcBmTemplateManagerDlg::onContextTypeProperty() {
    std::wstring err;
    m_impl->m_treeViewModel.doShowTypeProperty(&err);
    m_impl->showErrorIfNeeded(err);
}

void ZcBmTemplateManagerDlg::onContextCopyType() {
    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doCopyType(&err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onContextDeleteType() {
    std::wstring err;
    const bool ok = m_impl->m_treeViewModel.doDeleteType(&err);
    m_impl->showErrorIfNeeded(err);
    if (ok) {
        m_impl->refreshTree(m_impl->m_treeCtrl, m_impl->m_buttonPlace);
    }
}

void ZcBmTemplateManagerDlg::onContextSave() {
    std::wstring err;
    m_impl->m_treeViewModel.doSave(&err);
    m_impl->showErrorIfNeeded(err);
}

void ZcBmTemplateManagerDlg::onContextRename() {
    HTREEITEM selectedItem = m_impl->m_treeCtrl.GetSelectedItem();
    if (selectedItem != nullptr) {
        m_impl->m_treeCtrl.EditLabel(selectedItem);
    }
}

void ZcBmTemplateManagerDlg::onContextDelete() {
    const TemplateNode* focusedNode = m_impl->m_treeViewModel.focusedNode();
    if (focusedNode == nullptr) {
        return;
    }
    if (focusedNode->m_type == TemplateElementType::eTemplateType) {
        onContextDeleteType();
        return;
    }
    onContextBatchDelete();
}

BEGIN_MESSAGE_MAP(ZcBmTemplateManagerDlg, CZcUiDialog)
ON_EN_CHANGE(IDC_CB_SEARCH_EDIT, &ZcBmTemplateManagerDlg::onSearchTextChanged)
ON_BN_CLICKED(IDC_CB_BTN_TOGGLE_VISIBILITY, &ZcBmTemplateManagerDlg::onBnClickedToggleVisibility)
ON_BN_CLICKED(IDC_CB_BTN_LOAD, &ZcBmTemplateManagerDlg::onBnClickedLoad)
ON_BN_CLICKED(IDC_CB_BTN_PLACE, &ZcBmTemplateManagerDlg::onBnClickedPlace)
ON_NOTIFY(TVN_SELCHANGED, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeSelChanged)
ON_NOTIFY(NM_DBLCLK, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeDblClk)
ON_NOTIFY(NM_RCLICK, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeRClick)
ON_NOTIFY(NM_CUSTOMDRAW, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeCustomDraw)
ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeBeginLabelEdit)
ON_NOTIFY(TVN_ENDLABELEDIT, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeEndLabelEdit)
ON_NOTIFY(TVN_KEYDOWN, IDC_CB_TREE, &ZcBmTemplateManagerDlg::onTreeKeyDown)
ON_COMMAND(IDM_CB_BATCH_LOAD, &ZcBmTemplateManagerDlg::onContextBatchLoad)
ON_COMMAND(IDM_CB_BATCH_DELETE, &ZcBmTemplateManagerDlg::onContextBatchDelete)
ON_COMMAND(IDM_CB_NEW_TYPE, &ZcBmTemplateManagerDlg::onContextNewType)
ON_COMMAND(IDM_CB_EDIT_COMPONENT, &ZcBmTemplateManagerDlg::onContextEditTemplate)
ON_COMMAND(IDM_CB_PLACE, &ZcBmTemplateManagerDlg::onContextPlace)
ON_COMMAND(IDM_CB_TYPE_PROPERTY, &ZcBmTemplateManagerDlg::onContextTypeProperty)
ON_COMMAND(IDM_CB_COPY_TYPE, &ZcBmTemplateManagerDlg::onContextCopyType)
ON_COMMAND(IDM_CB_DELETE_TYPE, &ZcBmTemplateManagerDlg::onContextDeleteType)
ON_COMMAND(IDM_CB_SAVE, &ZcBmTemplateManagerDlg::onContextSave)
ON_COMMAND(IDM_CB_RENAME, &ZcBmTemplateManagerDlg::onContextRename)
ON_COMMAND(IDM_CB_DELETE, &ZcBmTemplateManagerDlg::onContextDelete)
END_MESSAGE_MAP()
