#include <Dialog/rename_template_dialog.h>
#include <Resources/mfc_rc.h>

IMPLEMENT_DYNAMIC(ZcBmRenameTemplateDialog, CZcUiDialog)

ZcBmRenameTemplateDialog::ZcBmRenameTemplateDialog(const CString& originalName, CWnd* pParent) : CZcUiDialog(IDD_MFC_RENAME_COMPONENT, pParent), m_parent(pParent), m_originalName(originalName) {}

CString ZcBmRenameTemplateDialog::getNewName() const { return m_newName; }

bool ZcBmRenameTemplateDialog::isCancelLoad() const { return m_cancelLoad; }

void ZcBmRenameTemplateDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_RENAME_EDIT_ORIGINAL, m_originalName);
    DDX_Text(pDX, IDC_RENAME_EDIT_NEW, m_newName);
}

BOOL ZcBmRenameTemplateDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    // if (auto* pIcon = dynamic_cast<CStatic*>(GetDlgItem(IDC_RENAME_ICON))) {
    //     pIcon->SetIcon(::LoadIcon(nullptr, IDI_WARNING));
    // }
    if (auto* pIcon = GetDlgItem(IDC_RENAME_ICON)) {
        auto hIcon = ::LoadIcon(nullptr, IDI_WARNING);
        static_cast<CStatic*>(pIcon)->SetIcon(hIcon);
    }

    UpdateData(FALSE);

    if (auto* pNewEdit = dynamic_cast<CEdit*>(GetDlgItem(IDC_RENAME_EDIT_NEW))) {
        pNewEdit->SetFocus();
        pNewEdit->SetSel(0, -1);
    }

    return FALSE;
}

void ZcBmRenameTemplateDialog::onBnClickedOk() {
    UpdateData(TRUE);

    m_newName.Trim();
    if (m_newName.IsEmpty()) {
        AfxMessageBox(_T("请输入新名称。"), MB_OK | MB_ICONWARNING);
        return;
    }

    if (m_newName.CompareNoCase(m_originalName) == 0) {
        AfxMessageBox(_T("新名称不能与原名称相同。"), MB_OK | MB_ICONWARNING);
        return;
    }

    m_cancelLoad = false;
    EndDialog(IDOK);
}

void ZcBmRenameTemplateDialog::onBnClickedCancelLoad() {
    m_cancelLoad = true;
    EndDialog(IDCANCEL);
}

BEGIN_MESSAGE_MAP(ZcBmRenameTemplateDialog, CZcUiDialog)
ON_BN_CLICKED(IDOK, &ZcBmRenameTemplateDialog::onBnClickedOk)
ON_BN_CLICKED(IDC_RENAME_BTN_CANCEL_LOAD, &ZcBmRenameTemplateDialog::onBnClickedCancelLoad)
END_MESSAGE_MAP()
