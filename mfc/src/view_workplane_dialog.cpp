#include <Dialog/view_workplane_dialog.h>
#include <Resources/mfc_rc.h>
#ifndef IDD_MFC_VIEW_WORKPLANE
#include "../inc/Resources/mfc_rc.h"
#endif
#include <Services/view_workplane_backend.h>
#include <acedads.h>
#include <afxcmn.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

IMPLEMENT_DYNAMIC(ZcBmViewWorkPlaneDialog, CZcUiDialog)

namespace {
constexpr int kGroupPlan = 1;
constexpr int kGroupElevation = 2;
constexpr int kGroupSection = 3;

CString workPlaneDisplayText(const WorkPlaneRef& plane) {
    if (!plane.name.empty()) {
        return CString(plane.name.c_str());
    }
    if (!plane.id.empty()) {
        return CString(plane.id.c_str());
    }
    return _T("拾取面");
}

bool isEmptyWorkPlane(const WorkPlaneRef& plane) { return plane.id.empty() && plane.name.empty(); }

bool isSameWorkPlane(const WorkPlaneRef& lhs, const WorkPlaneRef& rhs) { return lhs.id == rhs.id && lhs.name == rhs.name; }

bool isUiSupportedViewKind(BimViewKind kind) {
    return kind == BimViewKind::kPlan || kind == BimViewKind::kElevation || kind == BimViewKind::kSection;
}

int toGroupId(BimViewKind kind) {
    switch (kind) {
        case BimViewKind::kPlan:
            return kGroupPlan;
        case BimViewKind::kElevation:
            return kGroupElevation;
        case BimViewKind::kSection:
            return kGroupSection;
        default:
            return kGroupPlan;
    }
}

CString makeErrorMessage(const CString& title, const std::wstring& details) {
    CString message = title;
    if (!details.empty()) {
        message += _T("\n");
        message += CString(details.c_str());
    }
    return message;
}
}  // namespace

class ZcBmViewWorkPlaneDialogImpl {
   public:
    explicit ZcBmViewWorkPlaneDialogImpl(ZcBmViewWorkPlaneDialog& owner);

    void initDialog();
    void onListClick(NMHDR* pNMHDR, LRESULT* pResult);
    void onOkClicked();
    void onCancelClicked();

   private:
    struct ViewRowState {
        BimViewItem view;
        WorkPlaneRef originalWorkPlane;
        WorkPlaneRef currentWorkPlane;
        int itemIndex = -1;
    };

    void initListControl() const;
    bool loadRows();
    void renderRows();
    void addGroup(int groupId, const wchar_t* headerText) const;
    int rowIndexByItem(int itemIndex) const;
    bool pickWorkPlaneForRow(int rowIndex);
    void updatePendingChange(const ViewRowState& row);
    void showWarning(const CString& title, const std::wstring& details) const;

    ZcBmViewWorkPlaneDialog* m_owner = nullptr;
    std::shared_ptr<IViewWorkPlaneBackend> m_backend;
    std::vector<ViewRowState> m_rows;
    std::unordered_map<std::wstring, ViewWorkPlaneAssociation> m_pendingChanges;
};

ZcBmViewWorkPlaneDialogImpl::ZcBmViewWorkPlaneDialogImpl(ZcBmViewWorkPlaneDialog& owner)
    : m_owner(&owner), m_backend(createViewWorkPlaneBackend()) {}

void ZcBmViewWorkPlaneDialogImpl::initListControl() const {
    m_owner->m_viewList.SetExtendedStyle(m_owner->m_viewList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_LABELTIP);
    m_owner->m_viewList.InsertColumn(0, _T("当前视图"), LVCFMT_LEFT, 160);
    m_owner->m_viewList.InsertColumn(1, _T("关联工作平面"), LVCFMT_LEFT, 220);
    m_owner->m_viewList.EnableGroupView(TRUE);
}

bool ZcBmViewWorkPlaneDialogImpl::loadRows() {
    m_rows.clear();
    m_pendingChanges.clear();

    std::vector<BimViewItem> views;
    std::wstring errorMessage;
    if (!m_backend->listViews(&views, &errorMessage)) {
        showWarning(_T("获取视图失败。"), errorMessage);
        return false;
    }

    std::vector<std::wstring> viewIds;
    viewIds.reserve(views.size());
    for (const BimViewItem& view : views) {
        if (!isUiSupportedViewKind(view.kind)) {
            continue;
        }
        viewIds.push_back(view.id);
    }

    std::vector<ViewWorkPlaneAssociation> associations;
    errorMessage.clear();
    if (!m_backend->getAssociatedWorkPlanes(viewIds, &associations, &errorMessage) && !errorMessage.empty()) {
        showWarning(_T("读取关联工作平面失败。"), errorMessage);
    }

    std::unordered_map<std::wstring, WorkPlaneRef> associationByViewId;
    associationByViewId.reserve(associations.size());
    for (const ViewWorkPlaneAssociation& association : associations) {
        associationByViewId[association.viewId] = {association.workPlaneId, association.workPlaneName};
    }

    m_rows.reserve(viewIds.size());
    for (const BimViewItem& view : views) {
        if (!isUiSupportedViewKind(view.kind)) {
            continue;
        }

        ViewRowState row;
        row.view = view;
        const auto it = associationByViewId.find(view.id);
        if (it != associationByViewId.end()) {
            row.originalWorkPlane = it->second;
            row.currentWorkPlane = it->second;
        }
        m_rows.push_back(std::move(row));
    }
    return true;
}

void ZcBmViewWorkPlaneDialogImpl::addGroup(int groupId, const wchar_t* headerText) const {
    LVGROUP group = {};
    group.cbSize = sizeof(LVGROUP);
    group.mask = LVGF_GROUPID | LVGF_HEADER;
    group.iGroupId = groupId;
    group.pszHeader = const_cast<LPTSTR>(headerText);
    m_owner->m_viewList.InsertGroup(-1, &group);
}

void ZcBmViewWorkPlaneDialogImpl::renderRows() {
    m_owner->m_viewList.DeleteAllItems();
    m_owner->m_viewList.RemoveGroup(kGroupPlan);
    m_owner->m_viewList.RemoveGroup(kGroupElevation);
    m_owner->m_viewList.RemoveGroup(kGroupSection);

    addGroup(kGroupPlan, L"平面视图");
    addGroup(kGroupElevation, L"立面视图");
    addGroup(kGroupSection, L"剖面视图");

    for (size_t rowIndex = 0; rowIndex < m_rows.size(); ++rowIndex) {
        ViewRowState& row = m_rows[rowIndex];
        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_GROUPID;
        item.iItem = static_cast<int>(rowIndex);
        item.iSubItem = 0;
        item.pszText = const_cast<LPTSTR>(row.view.name.c_str());
        item.lParam = static_cast<LPARAM>(rowIndex);
        item.iGroupId = toGroupId(row.view.kind);
        const int itemIndex = m_owner->m_viewList.InsertItem(&item);
        row.itemIndex = itemIndex;
        m_owner->m_viewList.SetItemText(itemIndex, 1, workPlaneDisplayText(row.currentWorkPlane));
    }
}

void ZcBmViewWorkPlaneDialogImpl::initDialog() {
    initListControl();
    if (!loadRows()) {
        return;
    }
    renderRows();
}

int ZcBmViewWorkPlaneDialogImpl::rowIndexByItem(int itemIndex) const {
    if (itemIndex < 0 || itemIndex >= m_owner->m_viewList.GetItemCount()) {
        return -1;
    }
    const DWORD_PTR value = m_owner->m_viewList.GetItemData(itemIndex);
    if (value >= m_rows.size()) {
        return -1;
    }
    return static_cast<int>(value);
}

void ZcBmViewWorkPlaneDialogImpl::updatePendingChange(const ViewRowState& row) {
    if (isSameWorkPlane(row.currentWorkPlane, row.originalWorkPlane) || isEmptyWorkPlane(row.currentWorkPlane)) {
        m_pendingChanges.erase(row.view.id);
        return;
    }

    ViewWorkPlaneAssociation association;
    association.viewId = row.view.id;
    association.workPlaneId = row.currentWorkPlane.id;
    association.workPlaneName = row.currentWorkPlane.name;
    m_pendingChanges[row.view.id] = std::move(association);
}

void ZcBmViewWorkPlaneDialogImpl::showWarning(const CString& title, const std::wstring& details) const {
    AfxMessageBox(makeErrorMessage(title, details), MB_OK | MB_ICONWARNING);
}

bool ZcBmViewWorkPlaneDialogImpl::pickWorkPlaneForRow(int rowIndex) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(m_rows.size())) {
        return false;
    }

    ViewRowState& row = m_rows[static_cast<size_t>(rowIndex)];

    const bool wasVisible = m_owner->IsWindowVisible() != FALSE;
    CWnd* cadMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    if (wasVisible) {
        m_owner->ShowWindow(SW_HIDE);
    }
    if (cadMainWnd != nullptr) {
        cadMainWnd->SetForegroundWindow();
        cadMainWnd->SetActiveWindow();
        cadMainWnd->SetFocus();
    }

    WorkPlaneRef picked;
    std::wstring errorMessage;

    const bool pickedOk = m_backend->pickWorkPlaneForView(row.view.id, &picked, &errorMessage);

    if (wasVisible) {
        m_owner->ShowWindow(SW_SHOW);
    }
    m_owner->SetForegroundWindow();
    m_owner->SetActiveWindow();
    m_owner->m_viewList.SetFocus();

    if (!pickedOk) {
        if (!errorMessage.empty()) {
            showWarning(_T("拾取工作平面失败。"), errorMessage);
        }
        return false;
    }
    if (isEmptyWorkPlane(picked)) {
        return true;
    }

    row.currentWorkPlane = std::move(picked);
    m_owner->m_viewList.SetItemText(row.itemIndex, 1, workPlaneDisplayText(row.currentWorkPlane));
    updatePendingChange(row);

    return true;
}

void ZcBmViewWorkPlaneDialogImpl::onListClick(NMHDR* pNMHDR, LRESULT* pResult) {
    if (pResult != nullptr) {
        *pResult = 0;
    }
    if (pNMHDR == nullptr) {
        return;
    }

    auto* activate = reinterpret_cast<NMITEMACTIVATE*>(pNMHDR);
    if (activate->iItem < 0 || activate->iSubItem != 1) {
        return;
    }

    const int rowIndex = rowIndexByItem(activate->iItem);
    if (rowIndex < 0) {
        return;
    }
    pickWorkPlaneForRow(rowIndex);
}

void ZcBmViewWorkPlaneDialogImpl::onOkClicked() {
    std::vector<ViewWorkPlaneAssociation> changes;
    changes.reserve(m_pendingChanges.size());
    for (const auto& pair : m_pendingChanges) {
        const ViewWorkPlaneAssociation& item = pair.second;
        if (item.workPlaneId.empty() && item.workPlaneName.empty()) {
            continue;
        }
        changes.push_back(item);
    }

    if (!changes.empty()) {
        std::wstring errorMessage;
        if (!m_backend->setAssociatedWorkPlanes(changes, &errorMessage)) {
            showWarning(_T("提交关联工作平面失败。"), errorMessage);
            return;
        }
    }
    m_owner->DestroyWindow();
}

void ZcBmViewWorkPlaneDialogImpl::onCancelClicked() { m_owner->DestroyWindow(); }

ZcBmViewWorkPlaneDialog::ZcBmViewWorkPlaneDialog(CWnd* pParent)
    : CZcUiDialog(IDD_MFC_VIEW_WORKPLANE, pParent), m_parent(pParent), m_impl(std::make_unique<ZcBmViewWorkPlaneDialogImpl>(*this)) {}

ZcBmViewWorkPlaneDialog::~ZcBmViewWorkPlaneDialog() = default;

void ZcBmViewWorkPlaneDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_VWP_LIST, m_viewList);
}

BOOL ZcBmViewWorkPlaneDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }
    m_impl->initDialog();
    return TRUE;
}

void ZcBmViewWorkPlaneDialog::OnOK() { m_impl->onOkClicked(); }

void ZcBmViewWorkPlaneDialog::OnCancel() { m_impl->onCancelClicked(); }

void ZcBmViewWorkPlaneDialog::onBnClickedOk() { m_impl->onOkClicked(); }

void ZcBmViewWorkPlaneDialog::onBnClickedCancel() { m_impl->onCancelClicked(); }

void ZcBmViewWorkPlaneDialog::onNmClickViewList(NMHDR* pNMHDR, LRESULT* pResult) { m_impl->onListClick(pNMHDR, pResult); }

BEGIN_MESSAGE_MAP(ZcBmViewWorkPlaneDialog, CZcUiDialog)
ON_BN_CLICKED(IDOK, &ZcBmViewWorkPlaneDialog::onBnClickedOk)
ON_BN_CLICKED(IDCANCEL, &ZcBmViewWorkPlaneDialog::onBnClickedCancel)
ON_NOTIFY(NM_CLICK, IDC_VWP_LIST, &ZcBmViewWorkPlaneDialog::onNmClickViewList)
END_MESSAGE_MAP()
