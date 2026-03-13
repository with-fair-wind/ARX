#pragma once

enum class OverwriteChoice {
    kUpdateAll,
    kSkipParams,
    kCancelLoad,
};

class ZcBmOverwriteConfirmDialog : public CZcUiDialog {
   public:
    explicit ZcBmOverwriteConfirmDialog(CWnd* pParent = nullptr);
    ~ZcBmOverwriteConfirmDialog() override = default;
    ZcBmOverwriteConfirmDialog(const ZcBmOverwriteConfirmDialog&) = delete;
    ZcBmOverwriteConfirmDialog& operator=(const ZcBmOverwriteConfirmDialog&) = delete;
    ZcBmOverwriteConfirmDialog(ZcBmOverwriteConfirmDialog&&) = delete;
    ZcBmOverwriteConfirmDialog& operator=(ZcBmOverwriteConfirmDialog&&) = delete;

    [[nodiscard]] OverwriteChoice choice() const;

   protected:
    BOOL OnInitDialog() override;

    afx_msg void onBnClickedUpdateAll();
    afx_msg void onBnClickedSkipParams();
    afx_msg void onBnClickedCancelLoad();

    DECLARE_DYNAMIC(ZcBmOverwriteConfirmDialog)
    DECLARE_MESSAGE_MAP()

   private:
    CWnd* m_parent = nullptr;
    OverwriteChoice m_choice = OverwriteChoice::kCancelLoad;
};
