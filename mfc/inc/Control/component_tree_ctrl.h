#pragma once

#include <afxcmn.h>

#include <unordered_set>
#include <vector>

// 自定义树控件:
// - 承担多选语义（Ctrl/Shift）
// - 负责多选高亮绘制
// - 负责与树交互强相关的键鼠行为
class ZcBmComponentTreeCtrl : public CTreeCtrl {
   public:
    // 清理多选集合与锚点。
    void clearInteractionState();
    // 清空多选集合但保留其他交互状态。
    void resetMultiSelection();

    // 写入节点逻辑层级（用于同层级约束判断）。
    void setItemLogicalLevel(HTREEITEM hItem, int level);

    // 获取当前有效选中项列表（多选优先，单选回退）。
    [[nodiscard]] std::vector<HTREEITEM> getEffectiveSelectedItems() const;
    // 判断某项是否处于附加多选状态。
    [[nodiscard]] bool isItemMultiSelected(HTREEITEM hItem) const;

    // 处理单击选择逻辑（含 Ctrl/Shift）。
    void handleClick();
    // 处理右键选择修正并返回屏幕坐标。
    CPoint handleRightClickSelectionAdjust();
    // 处理多选项高亮绘制。
    void handleCustomDraw(NMTVCUSTOMDRAW* pCustomDraw, LRESULT* pResult);
    // 处理树自身快捷键（Ctrl+A/F2）。
    void handleKeyDown(const NMTVKEYDOWN* pKeyDown, LRESULT* pResult);

   protected:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()

   private:
    [[nodiscard]] bool isSameParentAndLevel(HTREEITEM left, HTREEITEM right) const;
    void selectItemRange(HTREEITEM hFrom, HTREEITEM hTo);
    void toggleItemSelection(HTREEITEM hItem);
    void setItemSelected(HTREEITEM hItem, bool selected);
    void flushRedraw();

    std::unordered_set<HTREEITEM> m_multiSelected;
    HTREEITEM m_lastClickedItem = nullptr;
};
