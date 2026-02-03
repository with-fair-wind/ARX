#include <Dialog/test_dialog.h>
#include <Resources/mfc_rc.h>

CTestDialog::CTestDialog(CWnd* pParent) : CDialog(IDD_MFC_TEST, pParent), m_parent(pParent) {}

BOOL CTestDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }
    return TRUE;
}

BEGIN_MESSAGE_MAP(CTestDialog, CDialog)
END_MESSAGE_MAP()
