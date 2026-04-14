#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using TemplateNodeId = std::uint64_t;
constexpr TemplateNodeId kInvalidTemplateNodeId = 0;

enum class TemplateElementType : std::int8_t {
    eUnknown = -1,
    eTopLevelTemplateCategory = 0,
    eSubTemplateCategory = 1,
    eTemplate = 2,
    eTemplateType = 3,
    eCount = 4,
};

enum class TemplateLoadState : std::uint8_t {
    kLoaded,
    kNotLoaded,
    kBuiltIn,
};

struct TemplateNode {
    TemplateNodeId m_nodeId = kInvalidTemplateNodeId;
    std::wstring m_name;
    TemplateElementType m_type = TemplateElementType::eUnknown;
    TemplateLoadState m_loadState = TemplateLoadState::kNotLoaded;
    bool m_isSystem = false;
    TemplateNodeId m_parentId = kInvalidTemplateNodeId;
    std::vector<TemplateNodeId> m_children;
};

struct TemplateNodeRoot {
    std::vector<TemplateNodeId> m_rootNodeIds;
    std::unordered_map<TemplateNodeId, TemplateNode> m_nodeIndex;
};

struct RenameValidation {
    bool valid = true;
    std::wstring message;
};

class ITemplateRepository {
   public:
    ITemplateRepository() = default;
    ITemplateRepository(const ITemplateRepository&) = default;
    ITemplateRepository& operator=(const ITemplateRepository&) = default;
    ITemplateRepository(ITemplateRepository&&) = default;
    ITemplateRepository& operator=(ITemplateRepository&&) = default;
    virtual ~ITemplateRepository() = default;
    virtual bool loadTree(TemplateNodeRoot* outTree, std::wstring* errorMessage) = 0;
    virtual bool search(const std::wstring& keyword, TemplateNodeRoot* outTree, std::wstring* errorMessage) = 0;
    virtual bool getNode(TemplateNodeId nodeId, TemplateNode* outNode) const = 0;
};

class ITemplateCommandService {
   public:
    ITemplateCommandService() = default;
    ITemplateCommandService(const ITemplateCommandService&) = default;
    ITemplateCommandService& operator=(const ITemplateCommandService&) = default;
    ITemplateCommandService(ITemplateCommandService&&) = default;
    ITemplateCommandService& operator=(ITemplateCommandService&&) = default;
    virtual ~ITemplateCommandService() = default;
    virtual bool batchLoad(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage) = 0;
    virtual bool batchDeleteOrUnload(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage) = 0;
    virtual bool createType(TemplateNodeId templateId, TemplateNodeId* newTypeId, std::wstring* errorMessage) = 0;
    virtual bool copyType(TemplateNodeId typeId, TemplateNodeId* newTypeId, std::wstring* errorMessage) = 0;
    virtual bool deleteType(TemplateNodeId typeId, std::wstring* errorMessage) = 0;
    virtual RenameValidation validateRename(TemplateNodeId nodeId, const std::wstring& newName) = 0;
    virtual bool rename(TemplateNodeId nodeId, const std::wstring& newName, std::wstring* errorMessage) = 0;
    virtual bool save(TemplateNodeId templateId, std::wstring* errorMessage) = 0;
    virtual bool place(TemplateNodeId typeId, std::wstring* errorMessage) = 0;
    virtual bool showTypeProperty(TemplateNodeId typeId, std::wstring* errorMessage) = 0;
    virtual bool editTemplate(TemplateNodeId templateId, std::wstring* errorMessage) = 0;
};

class ITemplatePermissionService {
   public:
    ITemplatePermissionService() = default;
    ITemplatePermissionService(const ITemplatePermissionService&) = default;
    ITemplatePermissionService& operator=(const ITemplatePermissionService&) = default;
    ITemplatePermissionService(ITemplatePermissionService&&) = default;
    ITemplatePermissionService& operator=(ITemplatePermissionService&&) = default;
    virtual ~ITemplatePermissionService() = default;
    virtual bool canPlace(TemplateNodeId nodeId) const = 0;
    virtual bool canEdit(TemplateNodeId nodeId) const = 0;
    virtual bool canDelete(TemplateNodeId nodeId) const = 0;
    virtual bool canUnload(TemplateNodeId nodeId) const = 0;
    virtual bool canSave(TemplateNodeId nodeId) const = 0;
    virtual bool canRename(TemplateNodeId nodeId) const = 0;
};

struct TemplateManagerBackendBundle {
    std::shared_ptr<ITemplateRepository> m_repository;
    std::shared_ptr<ITemplateCommandService> m_commandService;
    std::shared_ptr<ITemplatePermissionService> m_permissionService;
};

TemplateManagerBackendBundle createTemplateManagerBackend();
