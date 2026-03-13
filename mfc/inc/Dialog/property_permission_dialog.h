#pragma once

#include <memory>

class ZcBmPropertyPermissionDialogImpl;

class ZcBmPropertyPermissionDialog : public CZcUiDialog {
   public:
    explicit ZcBmPropertyPermissionDialog(CWnd* pParent = nullptr);
    ~ZcBmPropertyPermissionDialog() override;
    ZcBmPropertyPermissionDialog(const ZcBmPropertyPermissionDialog&) = delete;
    ZcBmPropertyPermissionDialog& operator=(const ZcBmPropertyPermissionDialog&) = delete;
    ZcBmPropertyPermissionDialog(ZcBmPropertyPermissionDialog&&) = delete;
    ZcBmPropertyPermissionDialog& operator=(ZcBmPropertyPermissionDialog&&) = delete;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void onBnClickedOk();
    afx_msg void onBnClickedMasterEditable();
    afx_msg void onNmClickPropertyList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onNmCustomdrawPropertyList(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_DYNAMIC(ZcBmPropertyPermissionDialog)
    DECLARE_MESSAGE_MAP()

   private:
    friend class ZcBmPropertyPermissionDialogImpl;

    CWnd* m_parent = nullptr;
    CListCtrl m_propertyList;
    CButton m_masterEditable;
    std::unique_ptr<ZcBmPropertyPermissionDialogImpl> m_impl;
};
