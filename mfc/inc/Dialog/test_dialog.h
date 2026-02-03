#pragma once

class CTestDialog : public CDialog {
   public:
    explicit CTestDialog(CWnd* pParent = nullptr);

   protected:
    BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

   private:
    CWnd* m_parent = nullptr;
};
