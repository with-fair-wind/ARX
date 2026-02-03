#include <Dialog/arxui_dialog.h>
#include <Resources/mfc_rc.h>

IMPLEMENT_DYNAMIC(CArxUiDemoDialog, CZcUiDialog)

CArxUiDemoDialog::CArxUiDemoDialog(CWnd* pParent) : CZcUiDialog(IDD_MFC_ARX_UI, pParent), m_parent(pParent) {}

void CArxUiDemoDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_ARXUI_EDIT_INPUT, m_input);
}

BOOL CArxUiDemoDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    m_input = _T("Hello ARX UI");
    UpdateData(FALSE);
    CheckDlgButton(IDC_ARXUI_CHECK_CMD, BST_CHECKED);
    return TRUE;
}

void CArxUiDemoDialog::OnBnClickedShow() {
    UpdateData(TRUE);

    CString text = m_input;
    if (text.IsEmpty()) {
        text = _T("(空)");
    }

    CString message;
    message.Format(_T("输入内容: %s"), text.GetString());

    if (IsDlgButtonChecked(IDC_ARXUI_CHECK_CMD) == BST_CHECKED) {
        acutPrintf(_T("\n[ARX UI] %s"), message.GetString());
    }

    AfxMessageBox(message, MB_OK | MB_ICONINFORMATION);
}

BEGIN_MESSAGE_MAP(CArxUiDemoDialog, CZcUiDialog)
ON_BN_CLICKED(IDC_ARXUI_BTN_SHOW, &CArxUiDemoDialog::OnBnClickedShow)
END_MESSAGE_MAP()
