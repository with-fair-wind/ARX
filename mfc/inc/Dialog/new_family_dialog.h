#pragma once

#include <Services/new_family_backend.h>

#include <memory>

class ZcBmNewFamilyDialogImpl;

class ZcBmNewFamilyDialog : public CZcUiDialog {
   public:
    explicit ZcBmNewFamilyDialog(CWnd* pParent = nullptr);
    ~ZcBmNewFamilyDialog() override;
    ZcBmNewFamilyDialog(const ZcBmNewFamilyDialog&) = delete;
    ZcBmNewFamilyDialog& operator=(const ZcBmNewFamilyDialog&) = delete;
    ZcBmNewFamilyDialog(ZcBmNewFamilyDialog&&) = delete;
    ZcBmNewFamilyDialog& operator=(ZcBmNewFamilyDialog&&) = delete;

    [[nodiscard]] const NewFamilyRequest& request() const;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void onCbnSelChangeCategory();
    afx_msg void onBnClickedBrowseTemplate();
    afx_msg void onBnClickedOk();

    DECLARE_DYNAMIC(ZcBmNewFamilyDialog)
    DECLARE_MESSAGE_MAP()

   private:
    friend class ZcBmNewFamilyDialogImpl;

    CWnd* m_parent = nullptr;
    CComboBox m_categoryCombo;
    CEdit m_templateEdit;
    CButton m_browseButton;
    CEdit m_familyNameEdit;
    std::unique_ptr<ZcBmNewFamilyDialogImpl> m_impl;
};
