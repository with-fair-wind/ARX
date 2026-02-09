#pragma once

#include <Services/load_to_doc_backend.h>

#include <vector>

class CLoadToDocDialog : public CZcUiDialog {
   public:
    explicit CLoadToDocDialog(CWnd* pParent = nullptr);
    ~CLoadToDocDialog() override = default;
    CLoadToDocDialog(const CLoadToDocDialog&) = delete;
    CLoadToDocDialog& operator=(const CLoadToDocDialog&) = delete;
    CLoadToDocDialog(CLoadToDocDialog&&) = delete;
    CLoadToDocDialog& operator=(CLoadToDocDialog&&) = delete;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnBnClickedOk();
    afx_msg void OnLvnItemChangedDocList(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_DYNAMIC(CLoadToDocDialog)
    DECLARE_MESSAGE_MAP()

   private:
    void refreshDocuments();
    void updateUiState();
    [[nodiscard]] int getCheckedCount() const;
    [[nodiscard]] std::vector<std::wstring> getCheckedDocIds() const;

    CWnd* m_parent = nullptr;
    CListCtrl m_docList;
    CButton m_chkPlace;
    CButton m_chkClose;
    std::vector<LoadToDocTarget> m_docs;
};
