#include <ViewModel/template_tree_view_model.h>

TemplateTreeViewModel::TemplateTreeViewModel(TemplateTreeModel& model) : m_model(&model) {}

void TemplateTreeViewModel::setKeyword(const std::wstring& keyword) { m_keyword = keyword; }

void TemplateTreeViewModel::setShowUnloaded(bool showUnloaded) { m_showUnloaded = showUnloaded; }

bool TemplateTreeViewModel::refresh(std::wstring* errorMessage) {
    if (m_model == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = L"视图模型未绑定模型层。";
        }
        return false;
    }
    const bool useSearch = !m_keyword.empty();
    return m_model->reload(m_keyword, useSearch, errorMessage);
}

std::vector<TemplateViewNode> TemplateTreeViewModel::buildViewTree() const {
    std::vector<TemplateViewNode> result;
    if (m_model == nullptr) {
        return result;
    }

    for (TemplateNodeId rootId : m_model->rootIds()) {
        TemplateViewNode root;
        if (buildVisibleNode(rootId, &root)) {
            result.push_back(std::move(root));
        }
    }
    return result;
}

void TemplateTreeViewModel::setSelection(std::vector<TemplateNodeId> selectedIds, TemplateNodeId focusedId) {
    m_selectedIds = std::move(selectedIds);
    m_focusedId = focusedId;
}

std::vector<TemplateNodeId> TemplateTreeViewModel::selectedIdsByType(TemplateElementType type) const {
    std::vector<TemplateNodeId> ids;
    if (m_model == nullptr) {
        return ids;
    }
    for (TemplateNodeId nodeId : m_selectedIds) {
        const TemplateNode* node = m_model->findNode(nodeId);
        if (node != nullptr && node->m_type == type) {
            ids.push_back(nodeId);
        }
    }
    return ids;
}

const TemplateNode* TemplateTreeViewModel::focusedNode() const {
    if (m_model == nullptr || m_focusedId == kInvalidTemplateNodeId) {
        return nullptr;
    }
    return m_model->findNode(m_focusedId);
}

bool TemplateTreeViewModel::doBatchLoad(std::wstring* errorMessage) {
    auto ids = selectedIdsByType(TemplateElementType::eTemplate);
    if (ids.empty()) {
        return false;
    }
    return m_model->batchLoad(ids, errorMessage);
}

bool TemplateTreeViewModel::doBatchDeleteOrUnload(std::wstring* errorMessage) {
    auto ids = selectedIdsByType(TemplateElementType::eTemplate);
    if (ids.empty()) {
        return false;
    }
    return m_model->batchDeleteOrUnload(ids, errorMessage);
}

bool TemplateTreeViewModel::doCreateType(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr || node->m_type != TemplateElementType::eTemplate) {
        return false;
    }
    TemplateNodeId newTypeId = kInvalidTemplateNodeId;
    return m_model->createType(node->m_nodeId, &newTypeId, errorMessage);
}

bool TemplateTreeViewModel::doEditTemplate(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr) {
        return false;
    }
    return m_model->editTemplate(node->m_nodeId, errorMessage);
}

bool TemplateTreeViewModel::doPlace(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr || node->m_type != TemplateElementType::eTemplateType) {
        return false;
    }
    return m_model->place(node->m_nodeId, errorMessage);
}

bool TemplateTreeViewModel::doShowTypeProperty(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr || node->m_type != TemplateElementType::eTemplateType) {
        return false;
    }
    return m_model->showTypeProperty(node->m_nodeId, errorMessage);
}

bool TemplateTreeViewModel::doCopyType(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr || node->m_type != TemplateElementType::eTemplateType) {
        return false;
    }
    TemplateNodeId newTypeId = kInvalidTemplateNodeId;
    return m_model->copyType(node->m_nodeId, &newTypeId, errorMessage);
}

bool TemplateTreeViewModel::doDeleteType(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr || node->m_type != TemplateElementType::eTemplateType) {
        return false;
    }
    return m_model->deleteType(node->m_nodeId, errorMessage);
}

bool TemplateTreeViewModel::doSave(std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr) {
        return false;
    }
    return m_model->save(node->m_nodeId, errorMessage);
}

bool TemplateTreeViewModel::doRename(const std::wstring& newName, std::wstring* errorMessage) {
    const TemplateNode* node = focusedNode();
    if (node == nullptr) {
        return false;
    }
    return m_model->rename(node->m_nodeId, newName, errorMessage);
}

bool TemplateTreeViewModel::canPlaceFocused() const {
    const TemplateNode* node = focusedNode();
    if (node == nullptr) {
        return false;
    }
    return node->m_type == TemplateElementType::eTemplateType && m_model->canPlace(node->m_nodeId);
}

bool TemplateTreeViewModel::canRenameFocused() const {
    const TemplateNode* node = focusedNode();
    if (node == nullptr) {
        return false;
    }
    return m_model->canRename(node->m_nodeId);
}

RenameValidation TemplateTreeViewModel::validateRenameFocused(const std::wstring& newName) const {
    const TemplateNode* node = focusedNode();
    if (node == nullptr) {
        RenameValidation invalid;
        invalid.valid = false;
        invalid.message = L"未选择目标节点。";
        return invalid;
    }
    return m_model->validateRename(node->m_nodeId, newName);
}

bool TemplateTreeViewModel::shouldShowNode(const TemplateNode& node) const {
    if (m_showUnloaded) {
        return true;
    }
    if (node.m_type == TemplateElementType::eTopLevelTemplateCategory || node.m_type == TemplateElementType::eSubTemplateCategory) {
        return true;
    }
    return node.m_loadState != TemplateLoadState::kNotLoaded;
}

bool TemplateTreeViewModel::buildVisibleNode(TemplateNodeId nodeId, TemplateViewNode* outNode) const {
    if (m_model == nullptr || outNode == nullptr) {
        return false;
    }

    const TemplateNode* node = m_model->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }

    std::vector<TemplateViewNode> visibleChildren;
    for (TemplateNodeId childId : node->m_children) {
        TemplateViewNode child;
        if (buildVisibleNode(childId, &child)) {
            visibleChildren.push_back(std::move(child));
        }
    }

    bool visible = shouldShowNode(*node);
    if (!visible && !visibleChildren.empty() &&
        (node->m_type == TemplateElementType::eTopLevelTemplateCategory || node->m_type == TemplateElementType::eSubTemplateCategory)) {
        visible = true;
    }

    if (!visible) {
        return false;
    }

    outNode->m_nodeId = nodeId;
    outNode->m_children = std::move(visibleChildren);
    return true;
}
