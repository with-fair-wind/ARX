#pragma once

#include <Control/component_tree_ctrl.h>

#include <memory>

class ZcBmComponentBrowserImpl;

// 构件浏览器主对话框（UI 外观层）。
//
// 设计说明:
// - 采用“薄对话框 + Impl”模式。
// - Dialog 负责消息映射和控件绑定，不直接承担复杂业务逻辑。
// - 业务交互（搜索、菜单命令）由 Impl 统一处理。
// - 树控件的多选与绘制行为由自定义 ZcBmComponentTreeCtrl 处理。
class ZcBmComponentBrowserDialog : public CZcUiDialog {
   public:
    // pParent 通常传 CAD 主窗口句柄。
    explicit ZcBmComponentBrowserDialog(CWnd* pParent = nullptr);
    ~ZcBmComponentBrowserDialog() override;
    ZcBmComponentBrowserDialog(const ZcBmComponentBrowserDialog&) = delete;
    ZcBmComponentBrowserDialog& operator=(const ZcBmComponentBrowserDialog&) = delete;
    ZcBmComponentBrowserDialog(ZcBmComponentBrowserDialog&&) = delete;
    ZcBmComponentBrowserDialog& operator=(ZcBmComponentBrowserDialog&&) = delete;

   protected:
    // MFC 标准控件绑定入口。
    void DoDataExchange(CDataExchange* pDX) override;
    // 初始化对话框: 绑定样式、加载树、初始化按钮状态。
    BOOL OnInitDialog() override;

    // 搜索关键字变更 -> 触发完整树/搜索树切换。
    afx_msg void onSearchTextChanged();
    // 显示/隐藏“未加载”节点。
    afx_msg void onBnClickedToggleVisibility();
    // 一键载入本地构件文件。
    afx_msg void onBnClickedLoad();
    // 布置当前选中类型。
    afx_msg void onBnClickedPlace();

    // 树选择变化 -> 更新布置按钮可用性。
    afx_msg void onTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
    // 双击树节点 -> 类型节点打开属性；类别/模板走控件默认展开收拢。
    afx_msg void onTreeDblClk(NMHDR* pNMHDR, LRESULT* pResult);
    // 右键树节点 -> 弹出分层级上下文菜单。
    afx_msg void onTreeRClick(NMHDR* pNMHDR, LRESULT* pResult);
    // 自定义绘制 -> 为“附加多选项”渲染高亮背景。
    afx_msg void onTreeCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    // 开始/结束标签编辑（重命名）。
    afx_msg void onTreeBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void onTreeEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    // 键盘快捷键处理（F2/Delete/Ctrl+A）。
    afx_msg void onTreeKeyDown(NMHDR* pNMHDR, LRESULT* pResult);

    // ===== 上下文菜单命令映射 =====
    afx_msg void onContextBatchLoad();
    afx_msg void onContextBatchDelete();
    afx_msg void onContextNewType();
    afx_msg void onContextEditComponent();
    afx_msg void onContextPlace();
    afx_msg void onContextTypeProperty();
    afx_msg void onContextCopyType();
    afx_msg void onContextDeleteType();
    afx_msg void onContextSave();
    afx_msg void onContextUnload();
    afx_msg void onContextRename();
    afx_msg void onContextDelete();

    DECLARE_DYNAMIC(ZcBmComponentBrowserDialog)
    DECLARE_MESSAGE_MAP()

   private:
    friend class ZcBmComponentBrowserImpl;

    // 父窗口，用于居中等 UI 行为。
    CWnd* m_parent = nullptr;
    // 搜索输入框。
    CEdit m_searchEdit;
    // 构件树控件（封装多选、绘制与基础交互）。
    ZcBmComponentTreeCtrl m_tree;
    // 工具按钮：隐藏/显示未加载。
    CButton m_btnToggleVisibility;
    // 工具按钮：一键载入。
    CButton m_btnLoad;
    // 工具按钮：布置。
    CButton m_btnPlace;
    // 逻辑实现层（PImpl）。
    std::unique_ptr<ZcBmComponentBrowserImpl> m_impl;
};
