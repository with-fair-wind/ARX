#pragma once

class CArxUiDemoDialog : public CZcUiDialog {
   public:
    explicit CArxUiDemoDialog(CWnd* pParent = nullptr);
    ~CArxUiDemoDialog() override = default;
    CArxUiDemoDialog(const CArxUiDemoDialog&) = delete;
    CArxUiDemoDialog& operator=(const CArxUiDemoDialog&) = delete;
    CArxUiDemoDialog(CArxUiDemoDialog&&) = delete;
    CArxUiDemoDialog& operator=(CArxUiDemoDialog&&) = delete;

   protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    afx_msg void OnBnClickedShow();

    DECLARE_DYNAMIC(CArxUiDemoDialog)
    DECLARE_MESSAGE_MAP()

   private:
    CWnd* m_parent = nullptr;
    CString m_input;
};
