#pragma once

#include <Services/template_manager_backend.h>
#include <afxcmn.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

class ZcBmTemplateTreeCtrl : public CTreeCtrl {
   public:
    void clearInteractionState();
    void resetMultiSelection();

    void bindItemType(HTREEITEM hItem, TemplateElementType type);
    void unbindItemType(HTREEITEM hItem);

    [[nodiscard]] TemplateNodeId itemNodeId(HTREEITEM hItem) const;
    [[nodiscard]] TemplateElementType itemType(HTREEITEM hItem) const;
    [[nodiscard]] std::vector<HTREEITEM> getEffectiveSelectedItems() const;
    [[nodiscard]] bool isItemMultiSelected(HTREEITEM hItem) const;

    void handleClick();
    CPoint handleRightClickSelectionAdjust();
    void handleCustomDraw(NMTVCUSTOMDRAW* pCustomDraw, LRESULT* pResult);
    void handleKeyDown(const NMTVKEYDOWN* pKeyDown, LRESULT* pResult);

   protected:
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()

   private:
    [[nodiscard]] bool isSameParentAndType(HTREEITEM leftItem, HTREEITEM rightItem) const;
    void toggleItemSelection(HTREEITEM hItem);
    void setItemSelected(HTREEITEM hItem, bool selected);
    void flushRedraw();

    std::unordered_set<HTREEITEM> m_multiSelected;
    std::unordered_map<HTREEITEM, TemplateElementType> m_itemTypeMap;
    HTREEITEM m_lastClickedItem = nullptr;
};
