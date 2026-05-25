#pragma once

#include <memory>

class ZcBmViewWorkPlaneDialogImpl;

class ZcBmViewWorkPlaneDialog : public CZcUiDialog {
   public:
    explicit ZcBmViewWorkPlaneDialog(CWnd* pParent = nullptr);
    ~ZcBmViewWorkPlaneDialog() override;
    ZcBmViewWorkPlaneDialog(const ZcBmViewWorkPlaneDialog&) = delete;
    ZcBmViewWorkPlaneDialog& operator=(const ZcBmViewWorkPlaneDialog&) = delete;
    ZcBmViewWorkPlaneDialog(ZcBmViewWorkPlaneDialog&&) = delete;
    ZcBmViewWorkPlaneDialog& operator=(ZcBmViewWorkPlaneDialog&&) = delete;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;

    afx_msg void onNmClickViewList(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_DYNAMIC(ZcBmViewWorkPlaneDialog)
    DECLARE_MESSAGE_MAP()

   private:
    friend class ZcBmViewWorkPlaneDialogImpl;

    CWnd* m_parent = nullptr;
    CListCtrl m_viewList;
    std::unique_ptr<ZcBmViewWorkPlaneDialogImpl> m_impl;
};
