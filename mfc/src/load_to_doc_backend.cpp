#include <Services/load_to_doc_backend.h>
#include <acdocman.h>
#include <acedads.h>

#include <cwctype>
#include <memory>
#include <string>
#include <vector>

namespace {
struct DocEntry {
    AcApDocument* doc = nullptr;
    std::wstring file_name;
    std::wstring title;
};

std::wstring safeToWString(const ZTCHAR* text) {
    if (text == nullptr) {
        return L"";
    }
    return std::wstring(text);
}

bool equalsIgnoreCase(const std::wstring& left, const std::wstring& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (size_t i = 0; i < left.size(); ++i) {
        if (::towlower(left[i]) != ::towlower(right[i])) {
            return false;
        }
    }
    return true;
}

std::vector<DocEntry> collectDocuments() {
    std::vector<DocEntry> docs;
    std::unique_ptr<AcApDocumentIterator> iterator(acDocManager->newAcApDocumentIterator());
    if (iterator == nullptr) {
        return docs;
    }

    for (; !iterator->done(); iterator->step()) {
        AcApDocument* doc = iterator->document();
        if (doc == nullptr) {
            continue;
        }

        DocEntry entry;
        entry.doc = doc;
        entry.file_name = safeToWString(doc->fileName());
        entry.title = safeToWString(doc->docTitle());
        docs.push_back(std::move(entry));
    }

    return docs;
}

int findDocumentIndex(const std::wstring& id, const std::vector<DocEntry>& docs, const std::vector<bool>& used) {
    for (size_t i = 0; i < docs.size(); ++i) {
        if (used[i]) {
            continue;
        }
        if ((!docs[i].file_name.empty() && equalsIgnoreCase(id, docs[i].file_name)) || (!docs[i].title.empty() && equalsIgnoreCase(id, docs[i].title))) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Zcad::ErrorStatus switchToPreferredPlanView(AcApDocument* doc) {
    if (doc == nullptr) {
        return Zcad::eInvalidInput;
    }
    // 约定：调用 CAD 命令切换到平面视图；若无平面视图则由 CAD 自行回退。
    // 如需更精确的视图选择，可替换为自有视图管理 API。
    // bWrapUpInactiveDoc=true 可在非活动文档上执行并尝试完成当前命令。
    return acDocManager->sendStringToExecute(doc, _T("_.PLAN\n"), true, true, true);
}

void setErrorMessage(std::wstring* error_message, const std::wstring& message) {
    if (error_message != nullptr) {
        *error_message = message;
    }
}

bool tryActivateInAppContext(AcApDocument* doc, std::wstring* error_message) {
    Zcad::ErrorStatus status = acDocManager->activateDocument(doc, false);
    if (status == Zcad::eOk) {
        return true;
    }

    // 处理特定错误类型
    if (status == Zcad::eDocumentSwitchDisabled) {
        setErrorMessage(error_message, L"当前状态下禁止切换文档（可能有命令正在执行或LISP活动）。");
        return false;
    }

    if (status == Zcad::eTargetDocNotQuiescent) {
        setErrorMessage(error_message, L"目标文档未处于静止状态，无法切换。");
        return false;
    }

    if (status == Zcad::eInvalidInput) {
        setErrorMessage(error_message, L"目标文档对象无效或已失效。");
        return false;
    }

    // 尝试备用方案
    Zcad::ErrorStatus fallback = acDocManager->setCurDocument(doc, ZcAp::kNone, false);
    if (fallback == Zcad::eOk) {
        return true;
    }

    setErrorMessage(error_message,
                    L"切换到目标文档失败。"
                    L"(activateDocument=" +
                        std::to_wstring(static_cast<int>(status)) + L", setCurDocument=" + std::to_wstring(static_cast<int>(fallback)) + L")");
    return false;
}

bool tryActivateInDocContext(AcApDocument* doc, std::wstring* error_message) {
    Zcad::ErrorStatus status = acDocManager->setCurDocument(doc, ZcAp::kNone, false);
    if (status == Zcad::eOk) {
        return true;
    }

    setErrorMessage(error_message, L"在当前上下文中无法切换文档。(status=" + std::to_wstring(static_cast<int>(status)) + L")");
    return false;
}

bool activateDocumentSafely(AcApDocument* doc, std::wstring* error_message) {
    if (doc == nullptr) {
        setErrorMessage(error_message, L"目标文档为空。");
        return false;
    }

    // 检查文档是否有效
    ZcDbDatabase* db = doc->database();
    if (db == nullptr) {
        setErrorMessage(error_message, L"目标文档数据库无效。");
        return false;
    }

    // 检查文档管理器状态
    if (acDocManager == nullptr) {
        setErrorMessage(error_message, L"文档管理器不可用。");
        return false;
    }

    // 根据上下文选择不同的激活策略
    if (acDocManager->isApplicationContext()) {
        return tryActivateInAppContext(doc, error_message);
    }

    return tryActivateInDocContext(doc, error_message);
}

AcApDocument* resolveDocumentFromDb(AcApDocument* doc) {
    if (doc == nullptr) {
        return nullptr;
    }
    ZcDbDatabase* db = doc->database();
    if (db == nullptr) {
        return doc;
    }
    AcApDocument* resolved = acDocManager->document(db);
    return resolved != nullptr ? resolved : doc;
}

bool resolveTargetDocuments(const std::vector<std::wstring>& doc_ids, const std::vector<DocEntry>& docs, std::vector<AcApDocument*>& targets, std::wstring* error_message) {
    if (doc_ids.empty()) {
        if (error_message != nullptr) {
            *error_message = L"未选择任何文档。";
        }
        return false;
    }

    targets.clear();
    targets.reserve(doc_ids.size());

    std::vector<bool> used(docs.size(), false);
    for (const auto& id : doc_ids) {
        const int index = findDocumentIndex(id, docs, used);
        if (index < 0) {
            if (error_message != nullptr) {
                *error_message = L"存在未匹配的文档，请刷新列表后重试。";
            }
            return false;
        }
        used[static_cast<size_t>(index)] = true;
        targets.push_back(docs[static_cast<size_t>(index)].doc);
    }

    if (targets.empty()) {
        if (error_message != nullptr) {
            *error_message = L"未匹配到任何文档。";
        }
        return false;
    }

    return true;
}

bool activateAndSwitchPlan(AcApDocument* target_doc, std::wstring* error_message) {
    if (target_doc == nullptr) {
        if (error_message != nullptr) {
            *error_message = L"目标文档无效。";
        }
        return false;
    }

    // 如果目标文档不是当前文档，需要先激活它
    if (target_doc != acDocManager->curDocument()) {
        if (!activateDocumentSafely(target_doc, error_message)) {
            return false;
        }
    }

    // 切换到平面视图
    const Zcad::ErrorStatus plan_status = switchToPreferredPlanView(target_doc);
    if (plan_status != Zcad::eOk) {
        if (error_message != nullptr) {
            *error_message = L"切换到平面视图失败。(PLAN=" + std::to_wstring(static_cast<int>(plan_status)) + L")";
        }
        return false;
    }

    return true;
}

void startPlaceComponentStub() {
    // TODO: 替换为实际的“布置构件”API
    acutPrintf(_T("\n[载入后布置构件] TODO: 调用布置构件API"));
}
}  // namespace

std::vector<LoadToDocTarget> getLoadToDocTargets() {
    std::vector<LoadToDocTarget> result;

    AcApDocument* current = acDocManager->curDocument();
    std::unique_ptr<AcApDocumentIterator> iterator(acDocManager->newAcApDocumentIterator());
    if (iterator == nullptr) {
        return result;
    }

    for (; !iterator->done(); iterator->step()) {
        AcApDocument* doc = iterator->document();
        if (doc == nullptr) {
            continue;
        }

        LoadToDocTarget target;
        const ZTCHAR* file_name = doc->fileName();
        const ZTCHAR* title = doc->docTitle();
        const bool has_file_name = file_name != nullptr && *file_name != 0;

        if (has_file_name) {
            target.id = file_name;
            target.name = file_name;
        } else if (title != nullptr && *title != 0) {
            target.id = title;
            target.name = title;
        } else {
            target.id = L"(未命名文档)";
            target.name = L"(未命名文档)";
        }

        target.is_current = (doc == current);
        result.push_back(std::move(target));
    }

    return result;
}

bool executeLoadToDocuments(const std::vector<std::wstring>& doc_ids, bool place_after_load, bool close_after_load, std::wstring* error_message) {
    AcApDocument* source_doc = acDocManager->curDocument();
    const auto docs = collectDocuments();
    if (docs.empty()) {
        if (error_message != nullptr) {
            *error_message = L"未找到可用文档。";
        }
        return false;
    }

    std::vector<AcApDocument*> targets;
    if (!resolveTargetDocuments(doc_ids, docs, targets, error_message)) {
        return false;
    }

    AcApDocument* target_doc = resolveDocumentFromDb(targets.front());
    if (!activateAndSwitchPlan(target_doc, error_message)) {
        return false;
    }

    if (place_after_load && targets.size() == 1) {
        startPlaceComponentStub();
    }

    if (close_after_load && source_doc != nullptr) {
        acDocManager->appContextCloseDocument(source_doc);
    }

    return true;
}
