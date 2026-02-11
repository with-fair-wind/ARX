#pragma once

#include <memory>

class ZcBmLoadToDocDialogImpl;

class ZcBmLoadToDocDialog : public CZcUiDialog {
   public:
    explicit ZcBmLoadToDocDialog(CWnd* pParent = nullptr);
    ~ZcBmLoadToDocDialog() override;
    ZcBmLoadToDocDialog(const ZcBmLoadToDocDialog&) = delete;
    ZcBmLoadToDocDialog& operator=(const ZcBmLoadToDocDialog&) = delete;
    ZcBmLoadToDocDialog(ZcBmLoadToDocDialog&&) = delete;
    ZcBmLoadToDocDialog& operator=(ZcBmLoadToDocDialog&&) = delete;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void onBnClickedOk();
    afx_msg void onLvnItemChangedDocList(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_DYNAMIC(ZcBmLoadToDocDialog)
    DECLARE_MESSAGE_MAP()

   private:
    friend class ZcBmLoadToDocDialogImpl;

    CWnd* m_parent = nullptr;
    CListCtrl m_docList;
    CButton m_chkPlace;
    CButton m_chkClose;
    std::unique_ptr<ZcBmLoadToDocDialogImpl> m_impl;
};
