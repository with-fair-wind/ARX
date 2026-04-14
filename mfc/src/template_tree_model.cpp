#include <Model/template_tree_model.h>

#include <utility>

TemplateTreeModel::TemplateTreeModel(std::shared_ptr<ITemplateRepository> repository, std::shared_ptr<ITemplateCommandService> commandService,
                                     std::shared_ptr<ITemplatePermissionService> permissionService)
    : m_repository(std::move(repository)), m_commandService(std::move(commandService)), m_permissionService(std::move(permissionService)) {}

bool TemplateTreeModel::reload(const std::wstring& keyword, bool useSearch, std::wstring* errorMessage) {
    if (m_repository == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = L"模板仓储服务不可用。";
        }
        return false;
    }
    if (useSearch) {
        return m_repository->search(keyword, &m_tree, errorMessage);
    }
    return m_repository->loadTree(&m_tree, errorMessage);
}

const TemplateNode* TemplateTreeModel::findNode(TemplateNodeId nodeId) const {
    std::unordered_map<TemplateNodeId, TemplateNode>::const_iterator it = m_tree.m_nodeIndex.find(nodeId);
    if (it == m_tree.m_nodeIndex.end()) {
        return nullptr;
    }
    return &it->second;
}

bool TemplateTreeModel::canPlace(TemplateNodeId nodeId) const { return m_permissionService != nullptr && m_permissionService->canPlace(nodeId); }
bool TemplateTreeModel::canEdit(TemplateNodeId nodeId) const { return m_permissionService != nullptr && m_permissionService->canEdit(nodeId); }
bool TemplateTreeModel::canDelete(TemplateNodeId nodeId) const { return m_permissionService != nullptr && m_permissionService->canDelete(nodeId); }
bool TemplateTreeModel::canSave(TemplateNodeId nodeId) const { return m_permissionService != nullptr && m_permissionService->canSave(nodeId); }
bool TemplateTreeModel::canRename(TemplateNodeId nodeId) const { return m_permissionService != nullptr && m_permissionService->canRename(nodeId); }

bool TemplateTreeModel::batchLoad(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->batchLoad(templateIds, errorMessage);
}
bool TemplateTreeModel::batchDeleteOrUnload(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->batchDeleteOrUnload(templateIds, errorMessage);
}
bool TemplateTreeModel::createType(TemplateNodeId templateId, TemplateNodeId* newTypeId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->createType(templateId, newTypeId, errorMessage);
}
bool TemplateTreeModel::copyType(TemplateNodeId typeId, TemplateNodeId* newTypeId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->copyType(typeId, newTypeId, errorMessage);
}
bool TemplateTreeModel::deleteType(TemplateNodeId typeId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->deleteType(typeId, errorMessage);
}
bool TemplateTreeModel::rename(TemplateNodeId nodeId, const std::wstring& newName, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->rename(nodeId, newName, errorMessage);
}
bool TemplateTreeModel::save(TemplateNodeId templateId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->save(templateId, errorMessage);
}
bool TemplateTreeModel::place(TemplateNodeId typeId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->place(typeId, errorMessage);
}
bool TemplateTreeModel::showTypeProperty(TemplateNodeId typeId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->showTypeProperty(typeId, errorMessage);
}
bool TemplateTreeModel::editTemplate(TemplateNodeId templateId, std::wstring* errorMessage) {
    return m_commandService != nullptr && m_commandService->editTemplate(templateId, errorMessage);
}

RenameValidation TemplateTreeModel::validateRename(TemplateNodeId nodeId, const std::wstring& newName) const {
    if (m_commandService == nullptr) {
        RenameValidation validation;
        validation.valid = false;
        validation.message = L"重命名服务不可用。";
        return validation;
    }
    return m_commandService->validateRename(nodeId, newName);
}
