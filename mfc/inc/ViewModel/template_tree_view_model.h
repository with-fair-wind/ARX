#pragma once

#include <Model/template_tree_model.h>

#include <vector>

struct TemplateViewNode {
    TemplateNodeId m_nodeId = kInvalidTemplateNodeId;
    std::vector<TemplateViewNode> m_children;
};

class TemplateTreeViewModel {
   public:
    explicit TemplateTreeViewModel(TemplateTreeModel& model);

    void setKeyword(const std::wstring& keyword);
    void setShowUnloaded(bool showUnloaded);
    [[nodiscard]] bool showUnloaded() const { return m_showUnloaded; }

    bool refresh(std::wstring* errorMessage);
    [[nodiscard]] std::vector<TemplateViewNode> buildViewTree() const;

    void setSelection(std::vector<TemplateNodeId> selectedIds, TemplateNodeId focusedId);
    [[nodiscard]] std::vector<TemplateNodeId> selectedIdsByType(TemplateElementType type) const;
    [[nodiscard]] const TemplateNode* focusedNode() const;

    bool doBatchLoad(std::wstring* errorMessage);
    bool doBatchDeleteOrUnload(std::wstring* errorMessage);
    bool doCreateType(std::wstring* errorMessage);
    bool doEditTemplate(std::wstring* errorMessage);
    bool doPlace(std::wstring* errorMessage);
    bool doShowTypeProperty(std::wstring* errorMessage);
    bool doCopyType(std::wstring* errorMessage);
    bool doDeleteType(std::wstring* errorMessage);
    bool doSave(std::wstring* errorMessage);
    bool doRename(const std::wstring& newName, std::wstring* errorMessage);

    [[nodiscard]] bool canPlaceFocused() const;
    [[nodiscard]] bool canRenameFocused() const;
    [[nodiscard]] RenameValidation validateRenameFocused(const std::wstring& newName) const;

   private:
    [[nodiscard]] bool shouldShowNode(const TemplateNode& node) const;
    [[nodiscard]] bool buildVisibleNode(TemplateNodeId nodeId, TemplateViewNode* outNode) const;

    TemplateTreeModel* m_model = nullptr;
    std::wstring m_keyword;
    bool m_showUnloaded = true;
    std::vector<TemplateNodeId> m_selectedIds;
    TemplateNodeId m_focusedId = kInvalidTemplateNodeId;
};
