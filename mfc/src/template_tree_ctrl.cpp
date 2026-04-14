#include <Control/template_tree_ctrl.h>

#include <cstdint>

BEGIN_MESSAGE_MAP(ZcBmTemplateTreeCtrl, CTreeCtrl)
ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void ZcBmTemplateTreeCtrl::clearInteractionState() {
    m_multiSelected.clear();
    m_itemTypeMap.clear();
    m_lastClickedItem = nullptr;
}

void ZcBmTemplateTreeCtrl::resetMultiSelection() { m_multiSelected.clear(); }

void ZcBmTemplateTreeCtrl::bindItemType(HTREEITEM hItem, TemplateElementType type) {
    if (hItem == nullptr) {
        return;
    }
    m_itemTypeMap[hItem] = type;
}

void ZcBmTemplateTreeCtrl::unbindItemType(HTREEITEM hItem) { m_itemTypeMap.erase(hItem); }

TemplateNodeId ZcBmTemplateTreeCtrl::itemNodeId(HTREEITEM hItem) const {
    if (hItem == nullptr) {
        return kInvalidTemplateNodeId;
    }
    return static_cast<TemplateNodeId>(GetItemData(hItem));
}

TemplateElementType ZcBmTemplateTreeCtrl::itemType(HTREEITEM hItem) const {
    auto it = m_itemTypeMap.find(hItem);
    if (it == m_itemTypeMap.end()) {
        return TemplateElementType::eUnknown;
    }
    return it->second;
}

std::vector<HTREEITEM> ZcBmTemplateTreeCtrl::getEffectiveSelectedItems() const {
    std::vector<HTREEITEM> result;
    if (m_multiSelected.empty()) {
        auto* hSel = GetSelectedItem();
        if (hSel != nullptr) {
            result.push_back(hSel);
        }
        return result;
    }
    result.assign(m_multiSelected.begin(), m_multiSelected.end());
    return result;
}

bool ZcBmTemplateTreeCtrl::isItemMultiSelected(HTREEITEM hItem) const { return m_multiSelected.count(hItem) > 0; }

bool ZcBmTemplateTreeCtrl::isSameParentAndType(HTREEITEM leftItem, HTREEITEM rightItem) const {
    if (leftItem == nullptr || rightItem == nullptr) {
        return false;
    }
    if (GetParentItem(leftItem) != GetParentItem(rightItem)) {
        return false;
    }
    return itemType(leftItem) == itemType(rightItem);
}

void ZcBmTemplateTreeCtrl::toggleItemSelection(HTREEITEM hItem) {
    if (isItemMultiSelected(hItem)) {
        m_multiSelected.erase(hItem);
    } else {
        setItemSelected(hItem, true);
    }
}

void ZcBmTemplateTreeCtrl::setItemSelected(HTREEITEM hItem, bool selected) {
    if (hItem == nullptr) {
        return;
    }
    if (selected) {
        m_multiSelected.insert(hItem);
    } else {
        m_multiSelected.erase(hItem);
    }
}

void ZcBmTemplateTreeCtrl::flushRedraw() { RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN); }

void ZcBmTemplateTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) {
    CTreeCtrl::OnLButtonDown(nFlags, point);
    handleClick();
}

void ZcBmTemplateTreeCtrl::handleClick() {
    SetRedraw(FALSE);

    CPoint pt;
    ::GetCursorPos(&pt);
    CPoint clientPt = pt;
    ScreenToClient(&clientPt);

    UINT flags = 0;
    HTREEITEM hHit = HitTest(clientPt, &flags);
    if (hHit == nullptr || (flags & static_cast<UINT>(TVHT_ONITEM)) == 0) {
        SetRedraw(TRUE);
        flushRedraw();
        return;
    }

    const bool ctrlDown = (static_cast<std::uint16_t>(::GetKeyState(VK_CONTROL)) & 0x8000U) != 0;
    const bool shiftDown = (static_cast<std::uint16_t>(::GetKeyState(VK_SHIFT)) & 0x8000U) != 0;

    if (!ctrlDown && !shiftDown) {
        resetMultiSelection();
        setItemSelected(hHit, true);
        SelectItem(hHit);
        m_lastClickedItem = hHit;
        SetRedraw(TRUE);
        flushRedraw();
        return;
    }

    auto applyFocusedSelection = [this, hHit]() {
        if (isItemMultiSelected(hHit)) {
            SelectItem(hHit);
            m_lastClickedItem = hHit;
            return;
        }
        if (!m_multiSelected.empty()) {
            auto* anchor = *m_multiSelected.begin();
            SelectItem(anchor);
            m_lastClickedItem = anchor;
            return;
        }
        SelectItem(nullptr);
        m_lastClickedItem = nullptr;
    };

    if (ctrlDown) {
        if (!m_multiSelected.empty()) {
            HTREEITEM anchor = *m_multiSelected.begin();
            if (!isSameParentAndType(anchor, hHit)) {
                resetMultiSelection();
                setItemSelected(hHit, true);
            } else {
                toggleItemSelection(hHit);
            }
        } else {
            setItemSelected(hHit, true);
        }
        applyFocusedSelection();
        SetRedraw(TRUE);
        flushRedraw();
        return;
    }

    if (shiftDown) {
        if (m_lastClickedItem == nullptr || !isSameParentAndType(m_lastClickedItem, hHit)) {
            resetMultiSelection();
            setItemSelected(hHit, true);
        } else {
            HTREEITEM parent = GetParentItem(hHit);
            HTREEITEM cursor = (parent == nullptr) ? GetRootItem() : GetChildItem(parent);
            std::vector<HTREEITEM> siblings;
            while (cursor != nullptr) {
                siblings.push_back(cursor);
                cursor = GetNextSiblingItem(cursor);
            }

            int fromIndex = -1;
            int toIndex = -1;
            for (int i = 0; i < static_cast<int>(siblings.size()); ++i) {
                if (siblings[static_cast<size_t>(i)] == m_lastClickedItem) {
                    fromIndex = i;
                }
                if (siblings[static_cast<size_t>(i)] == hHit) {
                    toIndex = i;
                }
            }

            if (fromIndex < 0 || toIndex < 0) {
                resetMultiSelection();
                setItemSelected(hHit, true);
            } else {
                const int begin = (fromIndex < toIndex) ? fromIndex : toIndex;
                const int end = (fromIndex < toIndex) ? toIndex : fromIndex;
                bool allSelected = true;
                for (int i = begin; i <= end; ++i) {
                    if (!isItemMultiSelected(siblings[static_cast<size_t>(i)])) {
                        allSelected = false;
                        break;
                    }
                }
                for (int i = begin; i <= end; ++i) {
                    setItemSelected(siblings[static_cast<size_t>(i)], !allSelected);
                }
            }
        }
        applyFocusedSelection();
        SetRedraw(TRUE);
        flushRedraw();
        return;
    }

    SetRedraw(TRUE);
    flushRedraw();
}

CPoint ZcBmTemplateTreeCtrl::handleRightClickSelectionAdjust() {
    CPoint pt;
    ::GetCursorPos(&pt);
    CPoint clientPt = pt;
    ScreenToClient(&clientPt);

    UINT flags = 0;
    HTREEITEM hHit = HitTest(clientPt, &flags);
    if (hHit != nullptr && (flags & static_cast<UINT>(TVHT_ONITEM)) != 0) {
        if (!isItemMultiSelected(hHit) && m_multiSelected.size() <= 1) {
            resetMultiSelection();
            SelectItem(hHit);
        }
    }
    return pt;
}

void ZcBmTemplateTreeCtrl::handleCustomDraw(NMTVCUSTOMDRAW* pCustomDraw, LRESULT* pResult) {
    if (pCustomDraw == nullptr || pResult == nullptr) {
        return;
    }

    if (pCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }

    if (pCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        auto* hItem = reinterpret_cast<HTREEITEM>(pCustomDraw->nmcd.dwItemSpec);
        HTREEITEM hFocused = GetSelectedItem();
        if (hItem != nullptr && hItem != hFocused && isItemMultiSelected(hItem)) {
            pCustomDraw->clrText = RGB(255, 255, 255);
            pCustomDraw->clrTextBk = RGB(0, 120, 215);
            *pResult = CDRF_NEWFONT;
            return;
        }
    }

    *pResult = CDRF_DODEFAULT;
}

void ZcBmTemplateTreeCtrl::handleKeyDown(const NMTVKEYDOWN* pKeyDown, LRESULT* pResult) {
    if (pResult != nullptr) {
        *pResult = 0;
    }
    if (pKeyDown == nullptr) {
        return;
    }

    const bool ctrlDown = (static_cast<std::uint16_t>(::GetKeyState(VK_CONTROL)) & 0x8000U) != 0;
    if (ctrlDown && pKeyDown->wVKey == 'A') {
        SetRedraw(FALSE);
        resetMultiSelection();
        HTREEITEM hItem = GetRootItem();
        while (hItem != nullptr) {
            setItemSelected(hItem, true);
            hItem = GetNextSiblingItem(hItem);
        }
        SetRedraw(TRUE);
        flushRedraw();
        return;
    }

    if (pKeyDown->wVKey == VK_F2) {
        auto* hSel = GetSelectedItem();
        if (hSel != nullptr) {
            EditLabel(hSel);
        }
    }
}
