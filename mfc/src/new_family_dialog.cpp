#include <Dialog/new_family_dialog.h>
#include <Resources/mfc_rc.h>
#ifndef IDD_MFC_NEW_FAMILY
#include "../inc/Resources/mfc_rc.h"
#endif
#include <acedads.h>
#include <aduiFileDialog.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

IMPLEMENT_DYNAMIC(ZcBmNewFamilyDialog, CZcUiDialog)

namespace {
std::wstring trimCopy(const std::wstring& text) {
    const std::wstring kWhitespace = L" \t\r\n";
    const std::size_t begin = text.find_first_not_of(kWhitespace);
    if (begin == std::wstring::npos) {
        return L"";
    }
    const std::size_t end = text.find_last_not_of(kWhitespace);
    return text.substr(begin, end - begin + 1);
}

std::wstring fileNameOnly(const std::wstring& pathOrName) {
    if (pathOrName.empty()) {
        return L"";
    }
    const std::size_t pos = pathOrName.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return pathOrName;
    }
    return pathOrName.substr(pos + 1);
}
}  // namespace

class ZcBmNewFamilyDialogImpl {
   public:
    explicit ZcBmNewFamilyDialogImpl(ZcBmNewFamilyDialog& owner);

    void initDialog();
    void onCategoryChanged();
    void onBrowseTemplateClicked();
    void onOkClicked();

    [[nodiscard]] const NewFamilyRequest& request() const { return m_request; }

   private:
    void fillCategoryCombo();
    void applyCategoryDefaultTemplate();
    [[nodiscard]] std::wstring getTemplateText() const;
    [[nodiscard]] std::wstring getFamilyNameText() const;
    [[nodiscard]] std::wstring getSelectedCategoryName() const;

    ZcBmNewFamilyDialog* m_owner = nullptr;
    std::vector<std::wstring> m_categories;
    NewFamilyRequest m_request;
    bool m_lockTemplateByCustomSelection = false;
};

ZcBmNewFamilyDialogImpl::ZcBmNewFamilyDialogImpl(ZcBmNewFamilyDialog& owner) : m_owner(&owner) {}

void ZcBmNewFamilyDialogImpl::fillCategoryCombo() {
    m_owner->m_categoryCombo.ResetContent();
    m_categories = listFamilyCategories();
    for (const auto& categoryName : m_categories) {
        m_owner->m_categoryCombo.AddString(categoryName.c_str());
    }

    if (!m_categories.empty()) {
        m_owner->m_categoryCombo.SetCurSel(0);
    }
}

void ZcBmNewFamilyDialogImpl::applyCategoryDefaultTemplate() {
    if (m_lockTemplateByCustomSelection) {
        return;
    }

    const std::wstring suggestion = suggestTemplateFileForCategoryName(getSelectedCategoryName());
    if (!suggestion.empty()) {
        m_owner->m_templateEdit.SetWindowTextW(suggestion.c_str());
    }
}

std::wstring ZcBmNewFamilyDialogImpl::getTemplateText() const {
    CString text;
    m_owner->m_templateEdit.GetWindowTextW(text);
    return trimCopy(std::wstring(text.GetString()));
}

std::wstring ZcBmNewFamilyDialogImpl::getFamilyNameText() const {
    CString text;
    m_owner->m_familyNameEdit.GetWindowTextW(text);
    return trimCopy(std::wstring(text.GetString()));
}

std::wstring ZcBmNewFamilyDialogImpl::getSelectedCategoryName() const {
    const int index = m_owner->m_categoryCombo.GetCurSel();
    if (index < 0 || index >= static_cast<int>(m_categories.size())) {
        return L"";
    }
    return m_categories[static_cast<size_t>(index)];
}

void ZcBmNewFamilyDialogImpl::initDialog() {
    std::wstring errorMessage;
    if (!initializeNewFamilyRules(&errorMessage)) {
        CString tip = _T("新建族规则初始化失败。");
        if (!errorMessage.empty()) {
            tip += _T("\n");
            tip += CString(errorMessage.c_str());
        }
        AfxMessageBox(tip, MB_OK | MB_ICONERROR);
    }

    fillCategoryCombo();
    applyCategoryDefaultTemplate();
}

void ZcBmNewFamilyDialogImpl::onCategoryChanged() { applyCategoryDefaultTemplate(); }

void ZcBmNewFamilyDialogImpl::onBrowseTemplateClicked() {
    CWnd* main_wnd = CWnd::FromHandle(adsw_acadMainWnd());
    CAdUiFileDialog dlg(TRUE, _T("ztf"), _T(""), OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, _T("模板文件 (*.ztf;*.dwg)|*.ztf;*.dwg|所有文件 (*.*)|*.*||"), main_wnd);
    if (dlg.DoModal() != IDOK) {
        return;
    }

    CString path = dlg.GetPathName();
    if (path.IsEmpty()) {
        return;
    }
    const std::wstring selectedPath = std::wstring(path.GetString());
    const bool isPredefined = isPredefinedTemplateFile(selectedPath);
    const std::wstring displayText = isPredefined ? fileNameOnly(selectedPath) : selectedPath;
    m_owner->m_templateEdit.SetWindowTextW(displayText.c_str());
    m_lockTemplateByCustomSelection = !isPredefined;
}

void ZcBmNewFamilyDialogImpl::onOkClicked() {
    NewFamilyRequest req;
    req.category_name = getSelectedCategoryName();
    req.template_path = getTemplateText();
    req.family_name = getFamilyNameText();
    if (req.template_path.empty()) {
        AfxMessageBox(_T("请选择样板文件。"), MB_OK | MB_ICONWARNING);
        return;
    }
    m_request = std::move(req);
    m_owner->EndDialog(IDOK);
}

ZcBmNewFamilyDialog::ZcBmNewFamilyDialog(CWnd* pParent) : CZcUiDialog(IDD_MFC_NEW_FAMILY, pParent), m_parent(pParent), m_impl(std::make_unique<ZcBmNewFamilyDialogImpl>(*this)) {}

ZcBmNewFamilyDialog::~ZcBmNewFamilyDialog() = default;

const NewFamilyRequest& ZcBmNewFamilyDialog::request() const { return m_impl->request(); }

void ZcBmNewFamilyDialog::DoDataExchange(CDataExchange* pDX) {
    CZcUiDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_NEW_FAMILY_CATEGORY_COMBO, m_categoryCombo);
    DDX_Control(pDX, IDC_NEW_FAMILY_TEMPLATE_EDIT, m_templateEdit);
    DDX_Control(pDX, IDC_NEW_FAMILY_TEMPLATE_BROWSE, m_browseButton);
    DDX_Control(pDX, IDC_NEW_FAMILY_NAME_EDIT, m_familyNameEdit);
}

BOOL ZcBmNewFamilyDialog::OnInitDialog() {
    CZcUiDialog::OnInitDialog();
    if (m_parent != nullptr) {
        CenterWindow(m_parent);
    }

    m_impl->initDialog();
    m_familyNameEdit.SetCueBanner(_T("选填项"), TRUE);
    m_templateEdit.SetFocus();
    return FALSE;
}

void ZcBmNewFamilyDialog::onCbnSelChangeCategory() { m_impl->onCategoryChanged(); }

void ZcBmNewFamilyDialog::onBnClickedBrowseTemplate() { m_impl->onBrowseTemplateClicked(); }

void ZcBmNewFamilyDialog::onBnClickedOk() { m_impl->onOkClicked(); }

BEGIN_MESSAGE_MAP(ZcBmNewFamilyDialog, CZcUiDialog)
ON_CBN_SELCHANGE(IDC_NEW_FAMILY_CATEGORY_COMBO, &ZcBmNewFamilyDialog::onCbnSelChangeCategory)
ON_BN_CLICKED(IDC_NEW_FAMILY_TEMPLATE_BROWSE, &ZcBmNewFamilyDialog::onBnClickedBrowseTemplate)
ON_BN_CLICKED(IDOK, &ZcBmNewFamilyDialog::onBnClickedOk)
END_MESSAGE_MAP()
