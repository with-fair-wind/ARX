#pragma once

#include <Services/template_manager_backend.h>

#include <memory>
#include <string>
#include <vector>

class TemplateTreeModel {
   public:
    TemplateTreeModel(std::shared_ptr<ITemplateRepository> repository, std::shared_ptr<ITemplateCommandService> commandService,
                      std::shared_ptr<ITemplatePermissionService> permissionService);

    bool reload(const std::wstring& keyword, bool useSearch, std::wstring* errorMessage);

    [[nodiscard]] const std::vector<TemplateNodeId>& rootIds() const { return m_tree.m_rootNodeIds; }
    [[nodiscard]] const TemplateNode* findNode(TemplateNodeId nodeId) const;

    [[nodiscard]] bool canPlace(TemplateNodeId nodeId) const;
    [[nodiscard]] bool canEdit(TemplateNodeId nodeId) const;
    [[nodiscard]] bool canDelete(TemplateNodeId nodeId) const;
    [[nodiscard]] bool canSave(TemplateNodeId nodeId) const;
    [[nodiscard]] bool canRename(TemplateNodeId nodeId) const;

    bool batchLoad(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage);
    bool batchDeleteOrUnload(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage);
    bool createType(TemplateNodeId templateId, TemplateNodeId* newTypeId, std::wstring* errorMessage);
    bool copyType(TemplateNodeId typeId, TemplateNodeId* newTypeId, std::wstring* errorMessage);
    bool deleteType(TemplateNodeId typeId, std::wstring* errorMessage);
    bool rename(TemplateNodeId nodeId, const std::wstring& newName, std::wstring* errorMessage);
    bool save(TemplateNodeId templateId, std::wstring* errorMessage);
    bool place(TemplateNodeId typeId, std::wstring* errorMessage);
    bool showTypeProperty(TemplateNodeId typeId, std::wstring* errorMessage);
    bool editTemplate(TemplateNodeId templateId, std::wstring* errorMessage);

    [[nodiscard]] RenameValidation validateRename(TemplateNodeId nodeId, const std::wstring& newName) const;

   private:
    std::shared_ptr<ITemplateRepository> m_repository;
    std::shared_ptr<ITemplateCommandService> m_commandService;
    std::shared_ptr<ITemplatePermissionService> m_permissionService;
    TemplateNodeRoot m_tree;
};
