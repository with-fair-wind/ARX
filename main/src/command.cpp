#include <Entity/testEntity.h>
#include <KTArxTool/KTArxCompare.h>
#include <KTArxTool/KTArxConvert.h>
#include <KTArxTool/KTArxCurve.h>
#include <KTArxTool/KTArxEntity.h>
#include <KTArxTool/KTArxUiPr.h>
#include <KTArxTool/KTArxUtility.h>
#include <Test/command.h>
#include <acdocman.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace {
constexpr double kBaseSize = 100.0;
constexpr double kDefaultZ = 0.0;
constexpr double kQuarterCircleDegrees = -90.0;
constexpr double kBulgeDivisor = 4.0;
constexpr int32_t kUpdatedTestNum = 1000;

struct SwitchContext {
    std::wstring sourceFilePath;
    std::wstring blockName;
    AcApDocument* targetDoc = nullptr;
    bool placeAfterLoad = false;
};

SwitchContext g_switchContext;

void CreatePolylineWithBulge(AcDbPolyline* polyline, const AcGePoint2dArray& points, const AcArray<double>& bulgeValues) {
    for (int i = 0; i < points.length(); i++) {
        polyline->addVertexAt(static_cast<unsigned int>(i), points.at(i), bulgeValues.at(i), 0, 0);
    }
}

std::wstring normalizePath(const std::wstring& input) {
    std::wstring result = input;
    std::replace(result.begin(), result.end(), L'\\', L'/');
    std::transform(result.begin(), result.end(), result.begin(), [](wchar_t chr) { return towlower(chr); });
    return result;
}

AcDbDatabase* findDatabaseByPath(const std::wstring& path) {
    if (path.empty()) {
        return nullptr;
    }

    std::wstring target = normalizePath(path);
    auto iterator = acDocManager->getDocumentIterator();
    if (iterator == nullptr) {
        return nullptr;
    }

    for (; !iterator->done(); iterator->step()) {
        AcApDocument* doc = iterator->document();
        if (doc == nullptr) {
            continue;
        }
        const ACHAR* fileName = doc->fileName();
        if (fileName == nullptr) {
            continue;
        }
        if (normalizePath(fileName) == target) {
            return doc->database();
        }
    }
    return nullptr;
}

AcDbObjectId getModelSpaceId(AcDbDatabase* database) {
    AcDbBlockTable* blockTable = nullptr;
    if (database->getBlockTable(blockTable, AcDb::kForRead) != Acad::eOk) {
        return AcDbObjectId::kNull;
    }

    AcDbObjectId modelSpaceId;
    blockTable->getAt(ACDB_MODEL_SPACE, modelSpaceId);
    blockTable->close();
    return modelSpaceId;
}

AcApDocument* selectTargetDocument() {
    auto iterator = acDocManager->getDocumentIterator();
    if (iterator == nullptr) {
        acutPrintf(_T("\n无法获取文档迭代器。"));
        return nullptr;
    }

    AcApDocument* currentDoc = acDocManager->curDocument();
    std::vector<AcApDocument*> otherDocs;

    for (; !iterator->done(); iterator->step()) {
        AcApDocument* doc = iterator->document();
        if (doc != nullptr && doc != currentDoc) {
            otherDocs.push_back(doc);
        }
    }

    if (otherDocs.empty()) {
        acutPrintf(_T("\n当前只有一个文档，无法切换。"));
        return nullptr;
    }

    acutPrintf(_T("\n--- 可切换的文档列表 ---"));
    for (size_t i = 0; i < otherDocs.size(); ++i) {
        const ACHAR* title = otherDocs[i]->docTitle();
        const ACHAR* fileName = otherDocs[i]->fileName();
        const ACHAR* display = (title != nullptr && *title != 0) ? title : fileName;
        acutPrintf(_T("\n  [%d] %s"), static_cast<int>(i + 1), display != nullptr ? display : _T("(未命名)"));
    }

    int selection = 0;
    if (acedGetInt(_T("\n请输入要切换到的文档编号: "), &selection) != RTNORM) {
        acutPrintf(_T("\n已取消。"));
        return nullptr;
    }

    if (selection < 1 || selection > static_cast<int>(otherDocs.size())) {
        acutPrintf(_T("\n编号无效。"));
        return nullptr;
    }

    return otherDocs[static_cast<size_t>(selection - 1)];
}

bool cloneBlockToTarget(const std::wstring& sourceFilePath, const std::wstring& blockName, AcApDocument* targetDoc) {
    AcDbDatabase* sourceDb = findDatabaseByPath(sourceFilePath);
    if (sourceDb == nullptr) {
        acutPrintf(_T("\n未找到源文档（可能已关闭）: %s"), sourceFilePath.c_str());
        return false;
    }

    AcDbBlockTable* srcBlockTable = nullptr;
    if (sourceDb->getBlockTable(srcBlockTable, AcDb::kForRead) != Acad::eOk) {
        acutPrintf(_T("\n获取源文档块表失败。"));
        return false;
    }

    AcDbObjectId srcBlockId;
    Acad::ErrorStatus status = srcBlockTable->getAt(blockName.c_str(), srcBlockId);
    srcBlockTable->close();
    if (status != Acad::eOk) {
        acutPrintf(_T("\n在源文档中未找到块定义: %s"), blockName.c_str());
        return false;
    }

    AcDbDatabase* targetDb = targetDoc->database();
    AcDbObjectId targetModelSpaceId = getModelSpaceId(targetDb);
    if (targetModelSpaceId == AcDbObjectId::kNull) {
        acutPrintf(_T("\n获取目标文档ModelSpace失败。"));
        return false;
    }

    AcDbObjectIdArray sourceIds;
    sourceIds.append(srcBlockId);

    AcDbIdMapping idMap;
    status = targetDb->wblockCloneObjects(sourceIds, targetModelSpaceId, idMap, AcDb::kDrcIgnore);
    if (status != Acad::eOk) {
        acutPrintf(_T("\n克隆块定义失败，错误码: %d"), static_cast<int>(status));
        return false;
    }

    acutPrintf(_T("\n块 \"%s\" 已载入目标文档。"), blockName.c_str());
    return true;
}

void appContextCallback(void* /*data*/) {
    AcApDocument* targetDoc = g_switchContext.targetDoc;
    const std::wstring sourceFilePath = g_switchContext.sourceFilePath;
    const std::wstring blockName = g_switchContext.blockName;
    bool placeAfterLoad = g_switchContext.placeAfterLoad;

    if (targetDoc == nullptr || sourceFilePath.empty() || blockName.empty()) {
        acutPrintf(_T("\n传递数据无效。"));
        return;
    }

    Acad::ErrorStatus status = acDocManager->setCurDocument(targetDoc, AcAp::kNone, false);
    if (status != Acad::eOk) {
        acutPrintf(_T("\n设置当前文档失败，错误码: %d"), static_cast<int>(status));
        return;
    }

    status = acDocManager->lockDocument(targetDoc, AcAp::kWrite);
    if (status != Acad::eOk) {
        acutPrintf(_T("\n锁定目标文档失败，错误码: %d"), static_cast<int>(status));
        return;
    }

    bool cloneOk = cloneBlockToTarget(sourceFilePath, blockName, targetDoc);

    acDocManager->unlockDocument(targetDoc);

    if (!cloneOk) {
        return;
    }

    if (!placeAfterLoad) {
        acutPrintf(_T("\n载入完成（不布置）。"));
        return;
    }

    status = acDocManager->activateDocument(targetDoc);
    if (status != Acad::eOk) {
        acutPrintf(_T("\n切换文档失败，错误码: %d"), static_cast<int>(status));
        return;
    }

    acDocManager->sendStringToExecute(targetDoc, _T("_test6_place\n"), false, false, false);
}

std::vector<std::wstring> collectUserBlockNames(AcDbDatabase* database) {
    std::vector<std::wstring> result;

    AcDbBlockTable* blockTable = nullptr;
    if (database->getBlockTable(blockTable, AcDb::kForRead) != Acad::eOk) {
        return result;
    }

    AcDbBlockTableIterator* iter = nullptr;
    if (blockTable->newIterator(iter) == Acad::eOk && iter != nullptr) {
        for (; !iter->done(); iter->step()) {
            AcDbBlockTableRecord* record = nullptr;
            if (iter->getRecord(record, AcDb::kForRead) != Acad::eOk) {
                continue;
            }
            ACHAR* name = nullptr;
            if (record->getName(name) == Acad::eOk && name != nullptr) {
                if (name[0] != _T('*')) {
                    result.emplace_back(name);
                }
                acutDelString(name);
            }
            record->close();
        }
        delete iter;
    }
    blockTable->close();
    return result;
}

void test7AppContextCallback(void* data) {
    auto* targetDoc = static_cast<AcApDocument*>(data);
    if (targetDoc == nullptr) {
        return;
    }

    acutPrintf(_T("\n[纤程] 当前处于应用级上下文"));
    acutPrintf(_T("\n[纤程] isApplicationContext = %d"), acDocManager->isApplicationContext());

    Acad::ErrorStatus status = acDocManager->activateDocument(targetDoc);
    acutPrintf(_T("\n[纤程] activateDocument 返回: %d"), static_cast<int>(status));

    acutPrintf(_T("\n[纤程] 尝试调用 acedInitGet..."));
    int initResult = acedInitGet(RSG_NONULL, _T("Yes No"));
    acutPrintf(_T("\n[纤程] acedInitGet 返回: %d"), initResult);

    acutPrintf(_T("\n[纤程] 尝试调用 acedGetKword..."));
    constexpr int kKeywordBufSize = 64;
    ACHAR keyword[kKeywordBufSize] = {};
    int kwResult = acedGetKword(_T("\n[纤程] 输入关键字 [Yes/No]: "), keyword);
    acutPrintf(_T("\n[纤程] acedGetKword 返回: %d, keyword=\"%s\""), kwResult, keyword);

    acutPrintf(_T("\n[纤程] 尝试调用 acedGetPoint..."));
    ads_point point;
    int ptResult = acedGetPoint(nullptr, _T("\n[纤程] 指定点: "), point);
    acutPrintf(_T("\n[纤程] acedGetPoint 返回: %d"), ptResult);

    acutPrintf(_T("\n[纤程] 尝试调用 acedGetInt..."));
    int intVal = 0;
    int intResult = acedGetInt(_T("\n[纤程] 输入整数: "), &intVal);
    acutPrintf(_T("\n[纤程] acedGetInt 返回: %d, val=%d"), intResult, intVal);

    acutPrintf(_T("\n[纤程] 所有测试完成。"));
}
}  // namespace

namespace Test_Command {
void test1() {
    const AcGePoint3d endPoint{kBaseSize, kBaseSize, kDefaultZ};
    auto line = std::make_unique<AcDbLine>(AcGePoint3d::kOrigin, endPoint);
    (void)KTArxTool::KTArxUtility::postToModelSpace(line.release());
}

void test2() {
    auto polyline = std::make_unique<AcDbPolyline>();  // 默认顶点数为0

    AcGePoint2dArray points;
    points.append(AcGePoint2d(0.0, 0.0));
    points.append(AcGePoint2d(kBaseSize, 0.0));
    points.append(AcGePoint2d(kBaseSize, kBaseSize));

    const double bulge = std::tan(KTArxTool::KTArxConvert::toRadian(kQuarterCircleDegrees / kBulgeDivisor));

    AcArray<double> bulges;
    bulges.append(0.0);
    bulges.append(1.0);
    bulges.append(bulge);  // 第三段为顺时针四分之一圆

    CreatePolylineWithBulge(polyline.get(), points, bulges);

    polyline->setClosed(true);
    (void)KTArxTool::KTArxUtility::postToModelSpace(polyline.release());
}

void test3() {
    auto entity = std::make_unique<Test_Entity::TestEntity>();
    (void)KTArxTool::KTArxUtility::postToModelSpace(entity.release());
}

void test4() {
    auto entity = std::make_unique<Test_Entity::TestEntity2>();
    (void)KTArxTool::KTArxUtility::postToModelSpace(entity.release());
}

void test5() {
    AcDbObjectId entId;
    AcGePoint3d pickedPoint;
    if (KTArxTool::KTArxUiPr::selEnt(_T("选择实体"), Test_Entity::TestEntity2::desc(), entId, pickedPoint)) {
        if (entId.isValid()) {
            AcDbObjectPointer<Test_Entity::TestEntity2> pTestEnt(entId, AcDb::kForWrite);
            {
                if (pTestEnt.openStatus() == Acad::eOk) {
                    // pTestEnt->setCenterPos(AcGePoint3d(100, 100, 0));
                    pTestEnt->setTestNum(kUpdatedTestNum);
                }
            }
        }
    }
}

void test6() {
    AcApDocument* currentDoc = acDocManager->curDocument();
    if (currentDoc == nullptr) {
        acutPrintf(_T("\n无法获取当前文档。"));
        return;
    }

    const ACHAR* srcFileName = currentDoc->fileName();
    if (srcFileName == nullptr || *srcFileName == 0) {
        acutPrintf(_T("\n当前文档未保存，无法获取文件路径。请先保存文档。"));
        return;
    }

    auto blockNames = collectUserBlockNames(currentDoc->database());
    if (blockNames.empty()) {
        acutPrintf(_T("\n当前文档中没有自定义块定义。"));
        return;
    }

    acutPrintf(_T("\n--- 可载入的块定义 ---"));
    for (size_t i = 0; i < blockNames.size(); ++i) {
        acutPrintf(_T("\n  [%d] %s"), static_cast<int>(i + 1), blockNames[i].c_str());
    }

    int selection = 0;
    if (acedGetInt(_T("\n请输入要载入的块编号: "), &selection) != RTNORM) {
        acutPrintf(_T("\n已取消。"));
        return;
    }

    if (selection < 1 || selection > static_cast<int>(blockNames.size())) {
        acutPrintf(_T("\n编号无效。"));
        return;
    }

    const std::wstring& selectedBlock = blockNames[static_cast<size_t>(selection - 1)];

    AcApDocument* targetDoc = selectTargetDocument();
    if (targetDoc == nullptr) {
        return;
    }

    int placeChoice = 0;
    acedGetInt(_T("\n载入后是否布置？(1=是, 0=否): "), &placeChoice);

    g_switchContext.sourceFilePath = srcFileName;
    g_switchContext.blockName = selectedBlock;
    g_switchContext.targetDoc = targetDoc;
    g_switchContext.placeAfterLoad = (placeChoice == 1);

    acDocManager->executeInApplicationContext(appContextCallback, nullptr);
}

void test6Place() {
    std::wstring blockName = g_switchContext.blockName;
    g_switchContext = {};

    if (blockName.empty()) {
        acutPrintf(_T("\n缺少块名称。"));
        return;
    }

    AcDbDatabase* targetDb = acdbCurDwg();
    AcDbBlockTable* blockTable = nullptr;
    if (targetDb->getBlockTable(blockTable, AcDb::kForRead) != Acad::eOk) {
        acutPrintf(_T("\n获取块表失败。"));
        return;
    }

    AcDbObjectId blockId;
    Acad::ErrorStatus status = blockTable->getAt(blockName.c_str(), blockId);
    blockTable->close();
    if (status != Acad::eOk) {
        acutPrintf(_T("\n未找到块定义: %s"), blockName.c_str());
        return;
    }

    acutPrintf(_T("\n--- 布置 \"%s\"：点击放置，右键/ESC退出 ---"), blockName.c_str());

    while (true) {
        ads_point point;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        if (acedGetPoint(nullptr, _T("\n指定插入点: "), point) != RTNORM) {
            break;
        }

        const AcGePoint3d insertPoint(point[X], point[Y], point[Z]);

        auto* blockRef = new AcDbBlockReference(insertPoint, blockId);
        AcDbObjectId refId = KTArxTool::KTArxUtility::postToModelSpace(blockRef);
        if (!refId.isValid()) {
            delete blockRef;
            acutPrintf(_T("\n插入块参照失败。"));
        }
    }

    acutPrintf(_T("\n布置完成。"));
}

void test7() {
    AcApDocument* targetDoc = selectTargetDocument();
    if (targetDoc == nullptr) {
        return;
    }

    acutPrintf(_T("\n[命令上下文] 即将进入纤程，测试各种 aced* 函数..."));
    acDocManager->executeInApplicationContext(test7AppContextCallback, targetDoc);
    acutPrintf(_T("\n[命令上下文] 纤程已返回。"));
}
}  // namespace Test_Command
