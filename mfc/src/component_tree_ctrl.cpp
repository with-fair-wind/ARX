#include <Control/component_tree_ctrl.h>

#include <cstdint>

namespace {
bool buildSiblingRange(CTreeCtrl& tree, HTREEITEM hFrom, HTREEITEM hTo, std::vector<HTREEITEM>& outRange) {
    outRange.clear();
    if (hFrom == nullptr || hTo == nullptr) {
        return false;
    }

    HTREEITEM parent = tree.GetParentItem(hFrom);
    HTREEITEM cursor = (parent == nullptr) ? tree.GetRootItem() : tree.GetChildItem(parent);

    std::vector<HTREEITEM> siblings;
    while (cursor != nullptr) {
        siblings.push_back(cursor);
        cursor = tree.GetNextSiblingItem(cursor);
    }

    int fromIndex = -1;
    int toIndex = -1;
    for (int i = 0; i < static_cast<int>(siblings.size()); ++i) {
        if (siblings[static_cast<size_t>(i)] == hFrom) {
            fromIndex = i;
        }
        if (siblings[static_cast<size_t>(i)] == hTo) {
            toIndex = i;
        }
    }

    if (fromIndex < 0 || toIndex < 0) {
        return false;
    }

    const int begin = (fromIndex < toIndex) ? fromIndex : toIndex;
    const int end = (fromIndex < toIndex) ? toIndex : fromIndex;
    for (int i = begin; i <= end; ++i) {
        outRange.push_back(siblings[static_cast<size_t>(i)]);
    }
    return true;
}
}  // namespace

BEGIN_MESSAGE_MAP(ZcBmComponentTreeCtrl, CTreeCtrl)
ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void ZcBmComponentTreeCtrl::clearInteractionState() {
    m_multiSelected.clear();
    m_lastClickedItem = nullptr;
}

void ZcBmComponentTreeCtrl::resetMultiSelection() { m_multiSelected.clear(); }

void ZcBmComponentTreeCtrl::setItemLogicalLevel(HTREEITEM hItem, int level) {
    if (hItem != nullptr) {
        SetItemData(hItem, static_cast<DWORD_PTR>(level));
    }
}

std::vector<HTREEITEM> ZcBmComponentTreeCtrl::getEffectiveSelectedItems() const {
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

bool ZcBmComponentTreeCtrl::isItemMultiSelected(HTREEITEM hItem) const { return m_multiSelected.count(hItem) > 0; }

bool ZcBmComponentTreeCtrl::isSameParentAndLevel(HTREEITEM left, HTREEITEM right) const {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    if (GetParentItem(left) != GetParentItem(right)) {
        return false;
    }
    return GetItemData(left) == GetItemData(right);
}

void ZcBmComponentTreeCtrl::selectItemRange(HTREEITEM hFrom, HTREEITEM hTo) {
    if (hFrom == nullptr || hTo == nullptr || !isSameParentAndLevel(hFrom, hTo)) {
        resetMultiSelection();
        setItemSelected(hTo, true);
        return;
    }

    std::vector<HTREEITEM> rangeItems;
    if (!buildSiblingRange(*this, hFrom, hTo, rangeItems)) {
        resetMultiSelection();
        setItemSelected(hTo, true);
        return;
    }

    resetMultiSelection();
    for (auto* item : rangeItems) {
        setItemSelected(item, true);
    }
}

void ZcBmComponentTreeCtrl::toggleItemSelection(HTREEITEM hItem) {
    if (isItemMultiSelected(hItem)) {
        m_multiSelected.erase(hItem);
    } else {
        setItemSelected(hItem, true);
    }
}

void ZcBmComponentTreeCtrl::setItemSelected(HTREEITEM hItem, bool selected) {
    if (hItem == nullptr) {
        return;
    }
    if (selected) {
        m_multiSelected.insert(hItem);
    } else {
        m_multiSelected.erase(hItem);
    }
}

void ZcBmComponentTreeCtrl::flushRedraw() {
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void ZcBmComponentTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) {
    CTreeCtrl::OnLButtonDown(nFlags, point);
    handleClick();
}

void ZcBmComponentTreeCtrl::handleClick() {
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

    const bool ctrlDown = (static_cast<uint16_t>(::GetKeyState(VK_CONTROL)) & 0x8000U) != 0;
    const bool shiftDown = (static_cast<uint16_t>(::GetKeyState(VK_SHIFT)) & 0x8000U) != 0;

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
            if (!isSameParentAndLevel(anchor, hHit)) {
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
        if (m_lastClickedItem == nullptr || !isSameParentAndLevel(m_lastClickedItem, hHit)) {
            resetMultiSelection();
            setItemSelected(hHit, true);
        } else {
            std::vector<HTREEITEM> rangeItems;
            if (!buildSiblingRange(*this, m_lastClickedItem, hHit, rangeItems)) {
                resetMultiSelection();
                setItemSelected(hHit, true);
            } else {
                bool allSelected = true;
                for (auto* item : rangeItems) {
                    if (!isItemMultiSelected(item)) {
                        allSelected = false;
                        break;
                    }
                }
                for (auto* item : rangeItems) {
                    setItemSelected(item, !allSelected);
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

CPoint ZcBmComponentTreeCtrl::handleRightClickSelectionAdjust() {
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

void ZcBmComponentTreeCtrl::handleCustomDraw(NMTVCUSTOMDRAW* pCustomDraw, LRESULT* pResult) {
    if (pResult == nullptr || pCustomDraw == nullptr) {
        return;
    }

    if (pCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        *pResult = CDRF_NOTIFYITEMDRAW;
        return;
    }

    if (pCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        auto hItem = reinterpret_cast<HTREEITEM>(pCustomDraw->nmcd.dwItemSpec);
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

void ZcBmComponentTreeCtrl::handleKeyDown(const NMTVKEYDOWN* pKeyDown, LRESULT* pResult) {
    if (pResult != nullptr) {
        *pResult = 0;
    }
    if (pKeyDown == nullptr) {
        return;
    }

    const bool ctrlDown = (static_cast<uint16_t>(::GetKeyState(VK_CONTROL)) & 0x8000U) != 0;

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
