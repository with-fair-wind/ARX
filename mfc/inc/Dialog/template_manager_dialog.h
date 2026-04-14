#pragma once

#include <memory>

class ZcBmTemplateManagerDlg : public CZcUiDialog {
   public:
    explicit ZcBmTemplateManagerDlg(CWnd* pParent = nullptr);
    ~ZcBmTemplateManagerDlg() override;

    ZcBmTemplateManagerDlg(const ZcBmTemplateManagerDlg&) = delete;
    ZcBmTemplateManagerDlg& operator=(const ZcBmTemplateManagerDlg&) = delete;
    ZcBmTemplateManagerDlg(ZcBmTemplateManagerDlg&&) = delete;
    ZcBmTemplateManagerDlg& operator=(ZcBmTemplateManagerDlg&&) = delete;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void onSearchTextChanged();
    afx_msg void onBnClickedToggleVisibility();
    afx_msg void onBnClickedLoad();
    afx_msg void onBnClickedPlace();

    afx_msg void onTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeRClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeKeyDown(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void onContextBatchLoad();
    afx_msg void onContextBatchDelete();
    afx_msg void onContextNewType();
    afx_msg void onContextEditTemplate();
    afx_msg void onContextPlace();
    afx_msg void onContextTypeProperty();
    afx_msg void onContextCopyType();
    afx_msg void onContextDeleteType();
    afx_msg void onContextSave();
    afx_msg void onContextRename();
    afx_msg void onContextDelete();

    DECLARE_DYNAMIC(ZcBmTemplateManagerDlg)
    DECLARE_MESSAGE_MAP()

   private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
