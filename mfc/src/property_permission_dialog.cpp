#include <Dialog/property_permission_dialog.h>
#include <Resources/mfc_rc.h>
#ifndef IDD_MFC_PROPERTY_PERMISSION
#include "../inc/Resources/mfc_rc.h"
#endif
#include <Services/property_permission_backend.h>
#include <afxcmn.h>
#include <uxtheme.h>
#include <vssym32.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

IMPLEMENT_DYNAMIC(ZcBmPropertyPermissionDialog, CZcUiDialog)
#pragma comment(lib, "UxTheme.lib")

namespace {
class ZcBmNewElevationPropertyDialog final : public CZcUiDialog {
   public:
    explicit ZcBmNewElevationPropertyDialog(CWnd* pParent = nullptr) : CZcUiDialog(IDD_MFC_NEW_ELEVATION_PROPERTY, pParent) {}
};
}  // namespace

class ZcBmPropertyPermissionDialogImpl {
   public:
    explicit ZcBmPropertyPermissionDialogImpl(ZcBmPropertyPermissionDialog& owner);
    void refreshProperties();
    void onMasterEditableClicked();
    void onListClick(NMHDR* pNMHDR, LRESULT* pResult);
    void onListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    void onOkClicked();

   private:
    [[nodiscard]] CRect calcCheckboxRect(int item_index) const;
    [[nodiscard]] bool hitTestEditableCheckbox(const CPoint& pt, int* item_index) const;
    void updateMasterCheckState();
    bool applyEditableForItem(int item_index, bool editable);
    void applyEditableForAll(bool editable);
    void drawEditableCheckbox(NMLVCUSTOMDRAW* custom_draw);

    ZcBmPropertyPermissionDialog* m_owner = nullptr;
    std::vector<PropertyPermissionItem> m_properties;
    bool m_ignoreMasterState = false;
};

ZcBmPropertyPermissionDialogImpl::ZcBmPropertyPermissionDialogImpl(ZcBmPropertyPermissionDialog& owner) : m_owner(&owner) {}

CRect ZcBmPropertyPermissionDialogImpl::calcCheckboxRect(int item_index) const {
    CRect cell_rect;
    m_owner->m_propertyList.GetSubItemRect(item_index, 1, LVIR_BOUNDS, cell_rect);

    constexpr int kCheckboxSize = 13;
    const int left = cell_rect.left + (cell_rect.Width() - kCheckboxSize) / 2;
    const int top = cell_rect.top + (cell_rect.Height() - kCheckboxSize) / 2;
    return CRect(left, top, left + kCheckboxSize, top + kCheckboxSize);
}

bool ZcBmPropertyPermissionDialogImpl::hitTestEditableCheckbox(const CPoint& pt, int* item_index) const {
    LVHITTESTINFO hit = {};
    hit.pt = pt;
    const int row = m_owner->m_propertyList.SubItemHitTest(&hit);
    if (row < 0 || hit.iSubItem != 1) {
        return false;
    }
    if (!calcCheckboxRect(row).PtInRect(pt)) {
        return false;
    }
    if (item_index != nullptr) {
        *item_index = row;
    }
    return true;
}

void ZcBmPropertyPermissionDialogImpl::updateMasterCheckState() {
    const auto checked_count = static_cast<int>(std::count_if(m_properties.begin(), m_properties.end(), [](const PropertyPermissionItem& item) { return item.editable; }));
    const auto total = static_cast<int>(m_properties.size());

    int state = BST_UNCHECKED;
    if (total > 0 && checked_count == total) {
        state = BST_CHECKED;
    } else if (checked_count > 0) {
        state = BST_INDETERMINATE;
    }

    m_ignoreMasterState = true;
    m_owner->m_masterEditable.SetCheck(state);
    m_ignoreMasterState = false;
}

bool ZcBmPropertyPermissionDialogImpl::applyEditableForItem(int item_index, bool editable) {
    if (item_index < 0 || item_index >= static_cast<int>(m_properties.size())) {
        return false;
    }

    auto& property = m_properties[static_cast<size_t>(item_index)];
    if (property.editable == editable) {
        return true;
    }

    std::wstring error_message;
    if (!setPropertyEditable(property.id, editable, &error_message)) {
        CString tip = _T("更新属性权限失败。");
        if (!error_message.empty()) {
            tip += _T("\n");
            tip += CString(error_message.c_str());
        }
        AfxMessageBox(tip, MB_OK | MB_ICONWARNING);
        return false;
    }

    property.editable = editable;
    return true;
}

void ZcBmPropertyPermissionDialogImpl::applyEditableForAll(bool editable) {
    for (int i = 0; i < static_cast<int>(m_properties.size()); ++i) {
        if (!applyEditableForItem(i, editable)) {
            break;
        }
    }
    m_owner->m_propertyList.Invalidate(FALSE);
    updateMasterCheckState();
}

void ZcBmPropertyPermissionDialogImpl::drawEditableCheckbox(NMLVCUSTOMDRAW* custom_draw) {
    if (custom_draw == nullptr) {
        return;
    }

    const int row = static_cast<int>(custom_draw->nmcd.dwItemSpec);
    if (row < 0 || row >= static_cast<int>(m_properties.size())) {
        return;
    }

    CDC* dc = CDC::FromHandle(custom_draw->nmcd.hdc);
    if (dc == nullptr) {
        return;
    }

    CRect subitem_rect;
    m_owner->m_propertyList.GetSubItemRect(row, 1, LVIR_BOUNDS, subitem_rect);
    const bool selected = (m_owner->m_propertyList.GetItemState(row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    const COLORREF bg_color = selected ? ::GetSysColor(COLOR_HIGHLIGHT) : ::GetSysColor(COLOR_WINDOW);
    dc->FillSolidRect(subitem_rect, bg_color);

    CRect checkbox_rect = calcCheckboxRect(row);
    const bool checked = m_properties[static_cast<size_t>(row)].editable;
    const bool enabled = m_owner->m_propertyList.IsWindowEnabled() != FALSE;

    if (HTHEME theme = ::OpenThemeData(m_owner->m_propertyList.GetSafeHwnd(), L"Button")) {
        const int state = checked ? (enabled ? CBS_CHECKEDNORMAL : CBS_CHECKEDDISABLED) : (enabled ? CBS_UNCHECKEDNORMAL : CBS_UNCHECKEDDISABLED);
        ::DrawThemeBackground(theme, custom_draw->nmcd.hdc, BP_CHECKBOX, state, &checkbox_rect, nullptr);
        ::CloseThemeData(theme);
        return;
    }

    UINT checkbox_state = DFCS_BUTTONCHECK;
    if (checked) {
        checkbox_state |= DFCS_CHECKED;
    }
    if (!enabled) {
        checkbox_state |= DFCS_INACTIVE;
    }
    dc->DrawFrameControl(checkbox_rect, DFC_BUTTON, checkbox_state);
}

void ZcBmPropertyPermissionDialogImpl::refreshProperties() {
    m_owner->m_propertyList.DeleteAllItems();
    m_properties = listPropertyPermissions();

    for (size_t i = 0; i < m_properties.size(); ++i) {
        const auto& item = m_properties[i];
        const int row = static_cast<int>(i);
        m_owner->m_propertyList.InsertItem(row, item.name.c_str());
        m_owner->m_propertyList.SetItemText(row, 1, _T(""));
    }
    updateMasterCheckState();
}

void ZcBmPropertyPermissionDialogImpl::onMasterEditableClicked() {
    if (m_ignoreMasterState) {
        return;
    }

    const int total = static_cast<int>(m_properties.size());
    const int checked_count = static_cast<int>(std::count_if(m_properties.begin(), m_properties.end(), [](const PropertyPermissionItem& item) { return item.editable; }));
    if (total > 0 && checked_count == total) {
        applyEditableForAll(false);
        return;
    }
    if (total > 0 && checked_count == 0) {
        applyEditableForAll(true);
        return;
    }
    auto status = m_owner->m_masterEditable.GetCheck();
    // 处于部分选时，点击总开关统一切到全选。
    applyEditableForAll(true);
}

void ZcBmPropertyPermissionDialogImpl::onListClick(NMHDR* pNMHDR, LRESULT* pResult) {
    if (pResult != nullptr) {
        *pResult = 0;
    }
    if (pNMHDR == nullptr) {
        return;
    }

    auto* click = (NMITEMACTIVATE*)pNMHDR;
    int row = -1;
    if (!hitTestEditableCheckbox(click->ptAction, &row)) {
        return;
    }

    const bool new_editable = !m_properties[static_cast<size_t>(row)].editable;
    if (!applyEditableForItem(row, new_editable)) {
        return;
    }
    m_owner->m_propertyList.InvalidateRect(calcCheckboxRect(row), FALSE);
    updateMasterCheckState();
}

void ZcBmPropertyPermissionDialogImpl::onListCustomDraw(NMHDR* pNMHDR, LRESULT* pResult) {
    if (pResult == nullptr || pNMHDR == nullptr) {
        return;
    }

    auto* custom_draw = (NMLVCUSTOMDRAW*)pNMHDR;
    if (custom_draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }
    if (custom_draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        return;
    }
    if (custom_draw->nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && custom_draw->iSubItem == 1) {
        drawEditableCheckbox(custom_draw);
        *pResult = CDRF_SKIPDEFAULT;
        return;
    }
    *pResult = CDRF_DODEFAULT;
}

void ZcBmPropertyPermissionDialogImpl::onOkClicked() {
    std::wstring error_message;
    if (!savePropertyPermissions(&error_message)) {
        CString tip = _T("保存属性权限失败。");
        if (!error_message.empty()) {
            tip += _T("\n");
            tip += CString(error_message.c_str());
        }
        AfxMessageBox(tip, MB_OK | MB_ICONWARNING);
        return;
    }
    m_owner->EndDialog(IDOK);
}

ZcBmPropertyPermissionDialog::ZcBmPropertyPermissionDialog(CWnd* pParent)
    : CZcUiDialog(IDD_MFC_PROPERTY_PERMISSION, pParent), m_parent(pParent), m_impl(std::make_unique<ZcBmPropertyPermissionDialogImpl>(*this)) {}

ZcBmPropertyPermissionDialog::~ZcBmPropertyPermissionDialog() = default;

void ZcBmPropertyPermissionDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PROPERTY_PERMISSION_LIST, m_propertyList);
    DDX_Control(pDX, IDC_PROPERTY_PERMISSION_MASTER, m_masterEditable);
}

BOOL ZcBmPropertyPermissionDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    m_propertyList.SetExtendedStyle(m_propertyList.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    m_propertyList.InsertColumn(0, _T("属性"), LVCFMT_LEFT, 190);
    m_propertyList.InsertColumn(1, _T("可编辑"), LVCFMT_CENTER, 60);

    m_impl->refreshProperties();
    return TRUE;
}

void ZcBmPropertyPermissionDialog::onBnClickedOk() { m_impl->onOkClicked(); }

void ZcBmPropertyPermissionDialog::onBnClickedMasterEditable() { m_impl->onMasterEditableClicked(); }

void ZcBmPropertyPermissionDialog::onBnClickedNewElevationProperty() {
    // 占位模态框：内网版本可在此弹窗中实现“新建标高属性”完整流程。
    ZcBmNewElevationPropertyDialog dialog(this);
    dialog.DoModal();
}

void ZcBmPropertyPermissionDialog::onNmClickPropertyList(NMHDR* pNMHDR, LRESULT* pResult) { m_impl->onListClick(pNMHDR, pResult); }

void ZcBmPropertyPermissionDialog::onNmCustomdrawPropertyList(NMHDR* pNMHDR, LRESULT* pResult) { m_impl->onListCustomDraw(pNMHDR, pResult); }

BEGIN_MESSAGE_MAP(ZcBmPropertyPermissionDialog, CZcUiDialog)
ON_BN_CLICKED(IDOK, &ZcBmPropertyPermissionDialog::onBnClickedOk)
ON_BN_CLICKED(IDC_PROPERTY_PERMISSION_MASTER, &ZcBmPropertyPermissionDialog::onBnClickedMasterEditable)
ON_BN_CLICKED(IDC_PROPERTY_PERMISSION_NEW_ELEVATION, &ZcBmPropertyPermissionDialog::onBnClickedNewElevationProperty)
ON_NOTIFY(NM_CLICK, IDC_PROPERTY_PERMISSION_LIST, &ZcBmPropertyPermissionDialog::onNmClickPropertyList)
ON_NOTIFY(NM_CUSTOMDRAW, IDC_PROPERTY_PERMISSION_LIST, &ZcBmPropertyPermissionDialog::onNmCustomdrawPropertyList)
END_MESSAGE_MAP()
