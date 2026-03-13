#include <Dialog/overwrite_confirm_dialog.h>
#include <Resources/mfc_rc.h>

IMPLEMENT_DYNAMIC(ZcBmOverwriteConfirmDialog, CZcUiDialog)

ZcBmOverwriteConfirmDialog::ZcBmOverwriteConfirmDialog(CWnd* pParent) : CZcUiDialog(IDD_MFC_OVERWRITE_CONFIRM, pParent), m_parent(pParent) {}

OverwriteChoice ZcBmOverwriteConfirmDialog::choice() const { return m_choice; }

BOOL ZcBmOverwriteConfirmDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    if (auto* pIcon = GetDlgItem(IDC_OVERWRITE_ICON)) {
        auto hIcon = ::LoadIcon(nullptr, IDI_QUESTION);
        static_cast<CStatic*>(pIcon)->SetIcon(hIcon);
    }

    return TRUE;
}

void ZcBmOverwriteConfirmDialog::onBnClickedUpdateAll() {
    m_choice = OverwriteChoice::kUpdateAll;
    EndDialog(IDOK);
}

void ZcBmOverwriteConfirmDialog::onBnClickedSkipParams() {
    m_choice = OverwriteChoice::kSkipParams;
    EndDialog(IDOK);
}

void ZcBmOverwriteConfirmDialog::onBnClickedCancelLoad() {
    m_choice = OverwriteChoice::kCancelLoad;
    EndDialog(IDCANCEL);
}

BEGIN_MESSAGE_MAP(ZcBmOverwriteConfirmDialog, CZcUiDialog)
ON_BN_CLICKED(IDC_OVERWRITE_BTN_ALL, &ZcBmOverwriteConfirmDialog::onBnClickedUpdateAll)
ON_BN_CLICKED(IDC_OVERWRITE_BTN_NO_PARAM, &ZcBmOverwriteConfirmDialog::onBnClickedSkipParams)
ON_BN_CLICKED(IDC_OVERWRITE_BTN_CANCEL, &ZcBmOverwriteConfirmDialog::onBnClickedCancelLoad)
END_MESSAGE_MAP()
