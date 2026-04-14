#include <Services/template_manager_backend.h>

#include <algorithm>
#include <cwctype>
#include <utility>

namespace {
TemplateNode makeNode(TemplateNodeId nodeId, const std::wstring& name, TemplateElementType type, TemplateLoadState state, bool isSystem, TemplateNodeId parentId) {
    TemplateNode node;
    node.m_nodeId = nodeId;
    node.m_name = name;
    node.m_type = type;
    node.m_loadState = state;
    node.m_isSystem = isSystem;
    node.m_parentId = parentId;
    return node;
}

std::wstring toLower(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return text;
}

bool containsIgnoreCase(const std::wstring& text, const std::wstring& keyword) {
    if (keyword.empty()) {
        return true;
    }
    return toLower(text).find(toLower(keyword)) != std::wstring::npos;
}

class StubTemplateRepository final : public ITemplateRepository {
   public:
    bool loadTree(TemplateNodeRoot* outTree, std::wstring* errorMessage) override {
        if (outTree == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = L"输出树对象为空。";
            }
            return false;
        }
        *outTree = createFullTree();
        return true;
    }

    bool search(const std::wstring& keyword, TemplateNodeRoot* outTree, std::wstring* errorMessage) override {
        if (outTree == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = L"输出树对象为空。";
            }
            return false;
        }
        if (keyword.empty()) {
            *outTree = createFullTree();
            return true;
        }

        const TemplateNodeRoot full = createFullTree();
        TemplateNodeRoot filtered;
        for (TemplateNodeId rootId : full.m_rootNodeIds) {
            collectFiltered(rootId, full, keyword, &filtered, kInvalidTemplateNodeId);
        }
        if (filtered.m_rootNodeIds.empty()) {
            if (errorMessage != nullptr) {
                errorMessage->clear();
            }
        }
        *outTree = std::move(filtered);
        return true;
    }

    bool getNode(TemplateNodeId nodeId, TemplateNode* outNode) const override {
        if (outNode == nullptr) {
            return false;
        }
        TemplateNodeRoot full = createFullTree();
        std::unordered_map<TemplateNodeId, TemplateNode>::const_iterator it = full.m_nodeIndex.find(nodeId);
        if (it == full.m_nodeIndex.end()) {
            return false;
        }
        *outNode = it->second;
        return true;
    }

   private:
    static void attachChild(TemplateNodeRoot* tree, TemplateNodeId parent, const TemplateNode& child) {
        tree->m_nodeIndex[child.m_nodeId] = child;
        tree->m_nodeIndex[parent].m_children.push_back(child.m_nodeId);
    }

    static TemplateNodeRoot createFullTree() {
        TemplateNodeRoot tree;

        const TemplateNode rootWin = makeNode(1, L"窗", TemplateElementType::eTopLevelTemplateCategory, TemplateLoadState::kLoaded, false, kInvalidTemplateNodeId);
        const TemplateNode rootWall = makeNode(2, L"墙", TemplateElementType::eTopLevelTemplateCategory, TemplateLoadState::kLoaded, false, kInvalidTemplateNodeId);
        tree.m_nodeIndex[rootWin.m_nodeId] = rootWin;
        tree.m_nodeIndex[rootWall.m_nodeId] = rootWall;
        tree.m_rootNodeIds.push_back(rootWin.m_nodeId);
        tree.m_rootNodeIds.push_back(rootWall.m_nodeId);

        const TemplateNode subWindowOpen = makeNode(11, L"平开窗", TemplateElementType::eSubTemplateCategory, TemplateLoadState::kLoaded, false, rootWin.m_nodeId);
        const TemplateNode subWindowSlide = makeNode(12, L"推拉窗", TemplateElementType::eSubTemplateCategory, TemplateLoadState::kLoaded, false, rootWin.m_nodeId);
        attachChild(&tree, rootWin.m_nodeId, subWindowOpen);
        attachChild(&tree, rootWin.m_nodeId, subWindowSlide);

        const TemplateNode tplSingle = makeNode(111, L"单扇平开窗", TemplateElementType::eTemplate, TemplateLoadState::kLoaded, false, subWindowOpen.m_nodeId);
        const TemplateNode tplDouble = makeNode(112, L"双扇平开窗", TemplateElementType::eTemplate, TemplateLoadState::kNotLoaded, false, subWindowOpen.m_nodeId);
        attachChild(&tree, subWindowOpen.m_nodeId, tplSingle);
        attachChild(&tree, subWindowOpen.m_nodeId, tplDouble);

        attachChild(&tree, tplSingle.m_nodeId, makeNode(1111, L"900*1200", TemplateElementType::eTemplateType, TemplateLoadState::kLoaded, false, tplSingle.m_nodeId));
        attachChild(&tree, tplSingle.m_nodeId, makeNode(1112, L"1200*1500", TemplateElementType::eTemplateType, TemplateLoadState::kLoaded, false, tplSingle.m_nodeId));
        attachChild(&tree, tplDouble.m_nodeId, makeNode(1121, L"1600*1500", TemplateElementType::eTemplateType, TemplateLoadState::kNotLoaded, false, tplDouble.m_nodeId));

        const TemplateNode tplSlide = makeNode(121, L"双轨推拉窗", TemplateElementType::eTemplate, TemplateLoadState::kBuiltIn, true, subWindowSlide.m_nodeId);
        attachChild(&tree, subWindowSlide.m_nodeId, tplSlide);
        attachChild(&tree, tplSlide.m_nodeId, makeNode(1211, L"1800*1200", TemplateElementType::eTemplateType, TemplateLoadState::kLoaded, true, tplSlide.m_nodeId));

        const TemplateNode subWallStruct = makeNode(21, L"结构墙", TemplateElementType::eSubTemplateCategory, TemplateLoadState::kLoaded, false, rootWall.m_nodeId);
        const TemplateNode subWallNonStruct = makeNode(22, L"非承重墙", TemplateElementType::eSubTemplateCategory, TemplateLoadState::kLoaded, false, rootWall.m_nodeId);
        attachChild(&tree, rootWall.m_nodeId, subWallStruct);
        attachChild(&tree, rootWall.m_nodeId, subWallNonStruct);

        const TemplateNode tplBasicWall = makeNode(211, L"基本墙", TemplateElementType::eTemplate, TemplateLoadState::kLoaded, true, subWallStruct.m_nodeId);
        attachChild(&tree, subWallStruct.m_nodeId, tplBasicWall);
        attachChild(&tree, tplBasicWall.m_nodeId, makeNode(2111, L"200mm", TemplateElementType::eTemplateType, TemplateLoadState::kLoaded, true, tplBasicWall.m_nodeId));
        attachChild(&tree, tplBasicWall.m_nodeId, makeNode(2112, L"240mm", TemplateElementType::eTemplateType, TemplateLoadState::kLoaded, true, tplBasicWall.m_nodeId));

        const TemplateNode tplPartition = makeNode(221, L"隔墙", TemplateElementType::eTemplate, TemplateLoadState::kNotLoaded, false, subWallNonStruct.m_nodeId);
        attachChild(&tree, subWallNonStruct.m_nodeId, tplPartition);
        attachChild(&tree, tplPartition.m_nodeId, makeNode(2211, L"100mm", TemplateElementType::eTemplateType, TemplateLoadState::kNotLoaded, false, tplPartition.m_nodeId));

        return tree;
    }

    static bool collectFiltered(TemplateNodeId sourceId, const TemplateNodeRoot& source, const std::wstring& keyword, TemplateNodeRoot* target, TemplateNodeId parentId) {
        std::unordered_map<TemplateNodeId, TemplateNode>::const_iterator sourceIt = source.m_nodeIndex.find(sourceId);
        if (sourceIt == source.m_nodeIndex.end()) {
            return false;
        }
        const TemplateNode& sourceNode = sourceIt->second;
        bool selfMatch = containsIgnoreCase(sourceNode.m_name, keyword);
        bool anyChildMatch = false;

        std::vector<TemplateNodeId> keptChildren;
        for (TemplateNodeId childId : sourceNode.m_children) {
            if (collectFiltered(childId, source, keyword, target, sourceNode.m_nodeId)) {
                keptChildren.push_back(childId);
                anyChildMatch = true;
            }
        }

        if (!selfMatch && !anyChildMatch) {
            return false;
        }

        TemplateNode copied = sourceNode;
        copied.m_parentId = parentId;
        copied.m_children.clear();
        target->m_nodeIndex[copied.m_nodeId] = copied;
        for (TemplateNodeId keptId : keptChildren) {
            target->m_nodeIndex[copied.m_nodeId].m_children.push_back(keptId);
        }

        if (parentId == kInvalidTemplateNodeId) {
            target->m_rootNodeIds.push_back(copied.m_nodeId);
        }
        return true;
    }
};

class StubTemplateCommandService final : public ITemplateCommandService {
   public:
    bool batchLoad(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage) override {
        static_cast<void>(templateIds);
        return notImplemented(errorMessage, L"[Stub] 批量载入尚未实现");
    }
    bool batchDeleteOrUnload(const std::vector<TemplateNodeId>& templateIds, std::wstring* errorMessage) override {
        static_cast<void>(templateIds);
        return notImplemented(errorMessage, L"[Stub] 批量删除/卸载尚未实现");
    }
    bool createType(TemplateNodeId templateId, TemplateNodeId* newTypeId, std::wstring* errorMessage) override {
        static_cast<void>(templateId);
        static_cast<void>(newTypeId);
        return notImplemented(errorMessage, L"[Stub] 新建类型尚未实现");
    }
    bool copyType(TemplateNodeId typeId, TemplateNodeId* newTypeId, std::wstring* errorMessage) override {
        static_cast<void>(typeId);
        static_cast<void>(newTypeId);
        return notImplemented(errorMessage, L"[Stub] 复制类型尚未实现");
    }
    bool deleteType(TemplateNodeId typeId, std::wstring* errorMessage) override {
        static_cast<void>(typeId);
        return notImplemented(errorMessage, L"[Stub] 删除类型尚未实现");
    }
    bool rename(TemplateNodeId nodeId, const std::wstring& newName, std::wstring* errorMessage) override {
        static_cast<void>(nodeId);
        static_cast<void>(newName);
        return notImplemented(errorMessage, L"[Stub] 重命名尚未实现");
    }
    bool save(TemplateNodeId templateId, std::wstring* errorMessage) override {
        static_cast<void>(templateId);
        return notImplemented(errorMessage, L"[Stub] 保存尚未实现");
    }
    bool place(TemplateNodeId typeId, std::wstring* errorMessage) override {
        static_cast<void>(typeId);
        return notImplemented(errorMessage, L"[Stub] 布置尚未实现");
    }
    bool showTypeProperty(TemplateNodeId typeId, std::wstring* errorMessage) override {
        static_cast<void>(typeId);
        return notImplemented(errorMessage, L"[Stub] 类型属性尚未实现");
    }
    bool editTemplate(TemplateNodeId templateId, std::wstring* errorMessage) override {
        static_cast<void>(templateId);
        return notImplemented(errorMessage, L"[Stub] 编辑模板尚未实现");
    }

    RenameValidation validateRename(TemplateNodeId nodeId, const std::wstring& newName) override {
        static_cast<void>(nodeId);
        RenameValidation validation;
        if (newName.empty()) {
            validation.valid = false;
            validation.message = L"名称不能为空";
            return validation;
        }
        const std::wstring illegal = L"#&?";
        for (wchar_t ch : newName) {
            if (illegal.find(ch) != std::wstring::npos) {
                validation.valid = false;
                validation.message = L"请输入不包含# & ? 的文本";
                return validation;
            }
        }
        return validation;
    }

   private:
    static bool notImplemented(std::wstring* errorMessage, const std::wstring& message) {
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        return false;
    }
};

class StubTemplatePermissionService final : public ITemplatePermissionService {
   public:
    bool canPlace(TemplateNodeId nodeId) const override {
        static_cast<void>(nodeId);
        return true;
    }
    bool canEdit(TemplateNodeId nodeId) const override {
        static_cast<void>(nodeId);
        return true;
    }
    bool canDelete(TemplateNodeId nodeId) const override {
        static_cast<void>(nodeId);
        return true;
    }
    bool canUnload(TemplateNodeId nodeId) const override {
        static_cast<void>(nodeId);
        return true;
    }
    bool canSave(TemplateNodeId nodeId) const override {
        static_cast<void>(nodeId);
        return true;
    }
    bool canRename(TemplateNodeId nodeId) const override {
        static_cast<void>(nodeId);
        return true;
    }
};
}  // namespace

TemplateManagerBackendBundle createTemplateManagerBackend() {
    TemplateManagerBackendBundle bundle;
    bundle.m_repository = std::make_shared<StubTemplateRepository>();
    bundle.m_commandService = std::make_shared<StubTemplateCommandService>();
    bundle.m_permissionService = std::make_shared<StubTemplatePermissionService>();
    return bundle;
}
