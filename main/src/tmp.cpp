#include <Windows.h>

namespace {
// 统一路径格式：转小写 + 正斜杠
std::wstring NormalizePath(const std::wstring& input) {
    std::wstring result = input;

    std::replace(result.begin(), result.end(), L'\\', L'/');

    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t c) { return towlower(c); });

    return result;
}
}  // namespace

AcDbDatabase* FindDatabaseByPath(const std::wstring& path) {
    if (path.empty()) return nullptr;

    std::wstring target = NormalizePath(path);

    AcApDocumentIterator* pIter = acDocManager->newAcApDocumentIterator();
    if (!pIter) return nullptr;

    AcDbDatabase* pResult = nullptr;

    for (; !pIter->done(); pIter->step()) {
        AcApDocument* pDoc = pIter->document();
        if (!pDoc) continue;

        const ACHAR* fileName = pDoc->fileName();
        if (!fileName) continue;

        std::wstring current = NormalizePath(fileName);

        if (current == target) {
            pResult = pDoc->database();
            break;
        }
    }

    delete pIter;
    return pResult;
}

AcDbDatabase* FindDatabaseByTitle(const std::wstring& title) {
    if (title.empty()) return nullptr;

    AcApDocumentIterator* pIter = acDocManager->newAcApDocumentIterator();
    if (!pIter) return nullptr;

    AcDbDatabase* pResult = nullptr;

    for (; !pIter->done(); pIter->step()) {
        AcApDocument* pDoc = pIter->document();
        if (!pDoc) continue;

        const ACHAR* docTitle = pDoc->docTitle();
        if (!docTitle) continue;

        std::wstring current = docTitle;

        if (_wcsicmp(current.c_str(), title.c_str()) == 0) {
            pResult = pDoc->database();
            break;
        }
    }

    delete pIter;
    return pResult;
}