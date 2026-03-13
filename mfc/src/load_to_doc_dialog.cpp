#include <Dialog/load_to_doc_dialog.h>
#include <Resources/mfc_rc.h>
#include <Services/load_to_doc_backend.h>
#include <afxcmn.h>

#include <vector>

IMPLEMENT_DYNAMIC(ZcBmLoadToDocDialog, CZcUiDialog)

class ZcBmLoadToDocDialogImpl {
   public:
    explicit ZcBmLoadToDocDialogImpl(ZcBmLoadToDocDialog& owner);
    void refreshDocuments();
    void updateUiState();
    void onOkClicked();
    void onDocListItemChanged(LRESULT* pResult);

    [[nodiscard]] const std::vector<std::wstring>& selectedDocIds() const { return m_selectedDocIds; }
    [[nodiscard]] bool placeAfterLoad() const { return m_placeAfterLoad; }
    [[nodiscard]] bool closeAfterLoad() const { return m_closeAfterLoad; }

   private:
    friend class ZcBmLoadToDocDialog;
    [[nodiscard]] int getCheckedCount() const;
    [[nodiscard]] std::vector<std::wstring> getCheckedDocIds() const;

    ZcBmLoadToDocDialog* m_owner = nullptr;
    std::vector<LoadToDocTarget> m_docs;
    std::vector<std::wstring> m_selectedDocIds;
    bool m_placeAfterLoad = false;
    bool m_closeAfterLoad = false;
};

ZcBmLoadToDocDialogImpl::ZcBmLoadToDocDialogImpl(ZcBmLoadToDocDialog& owner) : m_owner(&owner) {}

void ZcBmLoadToDocDialogImpl::refreshDocuments() {
    m_owner->m_docList.DeleteAllItems();
    m_docs.clear();

    const auto allDocs = getLoadToDocTargets();
    m_docs.reserve(allDocs.size());
    for (const auto& doc : allDocs) {
        if (doc.is_current) {
            continue;
        }
        m_docs.push_back(doc);
    }

    for (size_t i = 0; i < m_docs.size(); ++i) {
        const auto& doc = m_docs[i];
        m_owner->m_docList.InsertItem(static_cast<int>(i), doc.name.c_str());
    }

    if (m_docs.size() == 1) {
        m_owner->m_docList.SetCheck(0, TRUE);
    }
}

void ZcBmLoadToDocDialogImpl::updateUiState() {
    const int checkedCount = getCheckedCount();

    CWnd* pOk = m_owner->GetDlgItem(IDOK);
    if (pOk != nullptr) {
        pOk->EnableWindow(static_cast<BOOL>(checkedCount > 0));
    }

    if (checkedCount == 0) {
        m_owner->m_chkPlace.SetCheck(BST_UNCHECKED);
        m_owner->m_chkClose.SetCheck(BST_UNCHECKED);
    }

    if (checkedCount > 1) {
        m_owner->m_chkPlace.SetCheck(BST_UNCHECKED);
        m_owner->m_chkPlace.EnableWindow(FALSE);
    } else {
        m_owner->m_chkPlace.EnableWindow(TRUE);
    }

    m_owner->m_chkClose.EnableWindow(TRUE);
}

void ZcBmLoadToDocDialogImpl::onOkClicked() {
    const auto selectedIds = getCheckedDocIds();
    if (selectedIds.empty()) {
        updateUiState();
        return;
    }

    m_selectedDocIds = selectedIds;
    m_placeAfterLoad = m_owner->m_chkPlace.GetCheck() == BST_CHECKED;
    m_closeAfterLoad = m_owner->m_chkClose.GetCheck() == BST_CHECKED;
    m_owner->EndDialog(IDOK);
}

void ZcBmLoadToDocDialogImpl::onDocListItemChanged(LRESULT* pResult) {
    updateUiState();
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

int ZcBmLoadToDocDialogImpl::getCheckedCount() const {
    int count = 0;
    const int itemCount = m_owner->m_docList.GetItemCount();
    for (int i = 0; i < itemCount; ++i) {
        if (static_cast<bool>(m_owner->m_docList.GetCheck(i))) {
            ++count;
        }
    }
    return count;
}

std::vector<std::wstring> ZcBmLoadToDocDialogImpl::getCheckedDocIds() const {
    std::vector<std::wstring> result;
    const int itemCount = m_owner->m_docList.GetItemCount();
    for (int i = 0; i < itemCount; ++i) {
        if (static_cast<bool>(m_owner->m_docList.GetCheck(i))) {
            result.push_back(m_docs[static_cast<size_t>(i)].id);
        }
    }
    return result;
}

ZcBmLoadToDocDialog::ZcBmLoadToDocDialog(CWnd* pParent) : CZcUiDialog(IDD_MFC_LOAD_TO_DOC, pParent), m_parent(pParent), m_impl(std::make_unique<ZcBmLoadToDocDialogImpl>(*this)) {}

ZcBmLoadToDocDialog::~ZcBmLoadToDocDialog() = default;

const std::vector<std::wstring>& ZcBmLoadToDocDialog::selectedDocIds() const { return m_impl->selectedDocIds(); }
bool ZcBmLoadToDocDialog::placeAfterLoad() const { return m_impl->placeAfterLoad(); }
bool ZcBmLoadToDocDialog::closeAfterLoad() const { return m_impl->closeAfterLoad(); }

void ZcBmLoadToDocDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOAD_DOC_LIST, m_docList);
    DDX_Control(pDX, IDC_LOAD_AFTER_PLACE, m_chkPlace);
    DDX_Control(pDX, IDC_LOAD_AFTER_CLOSE, m_chkClose);
}

BOOL ZcBmLoadToDocDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    m_docList.SetExtendedStyle(m_docList.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    m_docList.InsertColumn(0, _T("选择需要载入的文档"), LVCFMT_LEFT, 230);

    m_impl->refreshDocuments();
    m_impl->updateUiState();
    return TRUE;
}

void ZcBmLoadToDocDialog::onBnClickedOk() { m_impl->onOkClicked(); }

void ZcBmLoadToDocDialog::onLvnItemChangedDocList(NMHDR* /*pNMHDR*/, LRESULT* pResult) { m_impl->onDocListItemChanged(pResult); }

BEGIN_MESSAGE_MAP(ZcBmLoadToDocDialog, CZcUiDialog)
ON_BN_CLICKED(IDOK, &ZcBmLoadToDocDialog::onBnClickedOk)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOAD_DOC_LIST, &ZcBmLoadToDocDialog::onLvnItemChangedDocList)
END_MESSAGE_MAP()
