#pragma once

class ZcBmRenameTemplateDialog : public CZcUiDialog {
   public:
    explicit ZcBmRenameTemplateDialog(const CString& originalName, CWnd* pParent = nullptr);
    ~ZcBmRenameTemplateDialog() override = default;
    ZcBmRenameTemplateDialog(const ZcBmRenameTemplateDialog&) = delete;
    ZcBmRenameTemplateDialog& operator=(const ZcBmRenameTemplateDialog&) = delete;
    ZcBmRenameTemplateDialog(ZcBmRenameTemplateDialog&&) = delete;
    ZcBmRenameTemplateDialog& operator=(ZcBmRenameTemplateDialog&&) = delete;

    [[nodiscard]] CString getNewName() const;
    [[nodiscard]] bool isCancelLoad() const;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void onBnClickedOk();
    afx_msg void onBnClickedCancelLoad();

    DECLARE_DYNAMIC(ZcBmRenameTemplateDialog)
    DECLARE_MESSAGE_MAP()

   private:
    CWnd* m_parent = nullptr;
    CString m_originalName;
    CString m_newName;
    bool m_cancelLoad = false;
};
