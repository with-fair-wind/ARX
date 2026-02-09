#include <Dialog/load_to_doc_dialog.h>
#include <Resources/mfc_rc.h>
#include <afxcmn.h>

IMPLEMENT_DYNAMIC(CLoadToDocDialog, CZcUiDialog)

CLoadToDocDialog::CLoadToDocDialog(CWnd* pParent) : CZcUiDialog(IDD_MFC_LOAD_TO_DOC, pParent), m_parent(pParent) {}

void CLoadToDocDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOAD_DOC_LIST, m_docList);
    DDX_Control(pDX, IDC_LOAD_AFTER_PLACE, m_chkPlace);
    DDX_Control(pDX, IDC_LOAD_AFTER_CLOSE, m_chkClose);
}

BOOL CLoadToDocDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    m_docList.SetExtendedStyle(m_docList.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    m_docList.InsertColumn(0, _T("选择需要载入的文档"), LVCFMT_LEFT, 230);

    refreshDocuments();
    updateUiState();
    return TRUE;
}

void CLoadToDocDialog::refreshDocuments() {
    m_docList.DeleteAllItems();
    m_docs.clear();

    const auto all_docs = getLoadToDocTargets();
    m_docs.reserve(all_docs.size());
    for (const auto& doc : all_docs) {
        if (doc.is_current) {
            continue;
        }
        m_docs.push_back(doc);
    }

    for (size_t i = 0; i < m_docs.size(); ++i) {
        const auto& doc = m_docs[i];
        m_docList.InsertItem(static_cast<int>(i), doc.name.c_str());
    }

    if (m_docs.size() == 1) {
        m_docList.SetCheck(0, TRUE);
    }
}

void CLoadToDocDialog::updateUiState() {
    const int checked_count = getCheckedCount();

    CWnd* pOk = GetDlgItem(IDOK);
    if (pOk != nullptr) {
        pOk->EnableWindow(static_cast<BOOL>(checked_count > 0));
    }

    if (checked_count == 0) {
        m_chkPlace.SetCheck(BST_UNCHECKED);
        m_chkClose.SetCheck(BST_UNCHECKED);
    }

    if (checked_count > 1) {
        m_chkPlace.SetCheck(BST_UNCHECKED);
        m_chkPlace.EnableWindow(FALSE);
    } else {
        m_chkPlace.EnableWindow(TRUE);
    }

    m_chkClose.EnableWindow(TRUE);
}

int CLoadToDocDialog::getCheckedCount() const {
    int count = 0;
    const int item_count = m_docList.GetItemCount();
    for (int i = 0; i < item_count; ++i) {
        if (static_cast<bool>(m_docList.GetCheck(i))) {
            ++count;
        }
    }
    return count;
}

std::vector<std::wstring> CLoadToDocDialog::getCheckedDocIds() const {
    std::vector<std::wstring> result;
    const int item_count = m_docList.GetItemCount();
    for (int i = 0; i < item_count; ++i) {
        if (static_cast<bool>(m_docList.GetCheck(i))) {
            result.push_back(m_docs[static_cast<size_t>(i)].id);
        }
    }
    return result;
}

void CLoadToDocDialog::OnBnClickedOk() {
    const auto selected_ids = getCheckedDocIds();
    if (selected_ids.empty()) {
        updateUiState();
        return;
    }

    // 模态对话框中与 CAD 交互：切换到编辑器命令环境
    BeginEditorCommand();

    std::wstring error;
    const bool ok = executeLoadToDocuments(selected_ids, m_chkPlace.GetCheck() == BST_CHECKED, m_chkClose.GetCheck() == BST_CHECKED, &error);
    if (!ok) {
        CancelEditorCommand();
        CString message(error.empty() ? _T("载入失败。") : error.c_str());
        AfxMessageBox(message, MB_OK | MB_ICONWARNING);
        return;
    }

    CompleteEditorCommand();
    EndDialog(IDOK);
}

void CLoadToDocDialog::OnLvnItemChangedDocList(NMHDR* /*pNMHDR*/, LRESULT* pResult) {
    updateUiState();
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

BEGIN_MESSAGE_MAP(CLoadToDocDialog, CZcUiDialog)
ON_BN_CLICKED(IDOK, &CLoadToDocDialog::OnBnClickedOk)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOAD_DOC_LIST, &CLoadToDocDialog::OnLvnItemChangedDocList)
END_MESSAGE_MAP()
