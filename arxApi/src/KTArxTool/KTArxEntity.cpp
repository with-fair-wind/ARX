#include <KTArxTool/KTArxConvert.h>
#include <KTArxTool/KTArxEntity.h>

#include <array>
#include <cassert>
#include <type_traits>

namespace KTArxTool {
bool KTArxEntity::ModifyEntByScale(AcDbEntity* entity, double scale, const AcGePoint3d& basePoint) {
    assert(entity != nullptr);
    AcGeMatrix3d mtx;
    mtx.setToScaling(scale, basePoint);
    return entity->transformBy(mtx) == Acad::eOk;
}

bool KTArxEntity::ModifyEntByScale(const AcDbObjectId& entityId, double scale, const AcGePoint3d& basePoint) {
    AcDbEntityPointer entity(entityId, AcDb::kForWrite);
    if (entity.openStatus() != Acad::eOk) {
        return false;
    }
    return ModifyEntByScale(entity, scale, basePoint);
}

bool KTArxEntity::ModifyEntByScale(const AcArray<AcDbEntity*>& entities, double scale, const AcGePoint3d& basePoint) {
    bool isOk = true;
    for (const auto& cur : entities) {
        isOk = ModifyEntByScale(cur, scale, basePoint);
        if (!isOk) {
            break;
        }
    }
    return isOk;
}

AcArray<AcDbEntity*> KTArxEntity::ExplodeEnt(AcDbEntity* entity) {
    AcArray<AcDbEntity*> arrExpEnt;
    AcDbVoidPtrArray exArr;
    if (entity != nullptr && entity->explode(exArr) == Acad::eOk) {
        for (int i = 0; i < exArr.length(); i++) {
            arrExpEnt.append(static_cast<AcDbEntity*>(exArr.at(i)));
        }
    }
    return arrExpEnt;
}

AcArray<AcDbEntity*> KTArxEntity::ExplodeEnt(const AcDbObjectId& entityId) {
    AcArray<AcDbEntity*> arrExpEnt;
    AcDbEntityPointer entity(entityId, AcDb::kForRead);
    if (Acad::eOk != entity.openStatus()) {
        return arrExpEnt;
    }
    return ExplodeEnt(entity);
}

bool KTArxEntity::MoveEnt(AcDbEntity* entity, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint) {
    assert(entity != nullptr);
    AcGeMatrix3d xform;
    AcGeVector3d vec(destPoint - basePoint);
    xform.setToTranslation(vec);
    return entity->transformBy(xform) == Acad::eOk;
}

bool KTArxEntity::MoveEnt(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint) {
    bool isOk = true;
    for (const auto& entity : entities) {
        if (entity == nullptr) {
            continue;
        }
        isOk = MoveEnt(entity, basePoint, destPoint);
        if (!isOk) {
            break;
        }
    }
    return isOk;
}

bool KTArxEntity::MoveEnt(const AcDbObjectId& entityId, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint) {
    AcDbEntityPointer entity(entityId, AcDb::kForWrite);
    if (Acad::eOk != entity.openStatus()) {
        return false;
    }
    return MoveEnt(entity, basePoint, destPoint);
}

bool KTArxEntity::MoveEnt(const AcDbObjectIdArray& entityIds, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint) {
    bool isOk = true;
    for (int i = 0; i < entityIds.length(); i++) {
        AcDbEntityPointer entity(entityIds[i], AcDb::kForWrite);
        if (Acad::eOk != entity.openStatus()) {
            continue;
        }
        isOk = MoveEnt(entity, basePoint, destPoint);
        if (!isOk) {
            break;
        }
    }
    return isOk;
}

bool KTArxEntity::RotateEnt(AcDbEntity* entity, const AcGePoint3d& basePoint, double rotation) {
    assert(entity != nullptr);
    AcGeMatrix3d xform;
    xform.setToRotation(rotation, AcGeVector3d::kZAxis, basePoint);  // 参数二是输入旋转轴向量
    return entity->transformBy(xform) == Acad::eOk;
}

bool KTArxEntity::RotateEnt(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& basePoint, double rotation) {
    bool isOk = true;
    for (const auto& entity : entities) {
        if (entity == nullptr) {
            continue;
        }
        isOk = RotateEnt(entity, basePoint, rotation);
        if (!isOk) {
            break;
        }
    }
    return isOk;
}

bool KTArxEntity::DeleteEnt(const AcDbObjectId& entityId) {
    AcDbEntityPointer entity(entityId, AcDb::kForWrite);
    if (Acad::eOk != entity.openStatus()) {
        return false;
    }
    return entity->erase() == Acad::eOk;
}

void KTArxEntity::DeleteEnt(AcArray<AcDbEntity*>& entities) {
    for (int i = 0; i < entities.length(); i++) {
        DEL(entities[i]);
    }
}

AcArray<AcDbEntity*> KTArxEntity::CopyEnt(const AcDbObjectIdArray& entityIds) {
    AcArray<AcDbEntity*> arrEnt;
    for (int i = (entityIds.length() - 1); i >= 0; i--) {
        AcDbEntityPointer entity(entityIds[i], AcDb::kForRead);
        if (Acad::eOk != entity.openStatus()) {
            continue;
        }
        AcDbEntity* pCopy = AcDbEntity::cast(entity->clone());
        if (pCopy != nullptr) {
            arrEnt.append(pCopy);
        }
    }
    return arrEnt;
}

AcArray<AcDbEntity*> KTArxEntity::CopyEnt(const AcArray<AcDbEntity*>& entities) {
    AcArray<AcDbEntity*> arrEnt;
    for (int i = (entities.length() - 1); i >= 0; i--) {
        AcDbEntity* const source = entities[i];
        if (source == nullptr) {
            continue;
        }
        AcDbEntity* pCopy = AcDbEntity::cast(source->clone());
        if (pCopy != nullptr) {
            arrEnt.append(pCopy);
        }
    }
    return arrEnt;
}

AcDbEntity* KTArxEntity::CopyEnt(const AcDbObjectId& entityId) {
    AcDbEntityPointer entity(entityId, AcDb::kForRead);
    if (Acad::eOk != entity.openStatus()) {
        return nullptr;
    }
    return AcDbEntity::cast(entity->clone());
}

AcDbEntity* KTArxEntity::MirrorEnt(AcDbEntity* entity, const AcGePoint3d& point1, const AcGePoint3d& point2) {
    if (entity == nullptr) {
        return nullptr;
    }

    AcGeMatrix3d mat;
    mat.setToMirroring(AcGeLine3d(point1, point2));
    AcDbEntity* pCopy = AcDbEntity::cast(entity->clone());
    if (pCopy == nullptr) {
        return nullptr;
    }
    pCopy->transformBy(mat);

    AcGeMatrix3d mat2;
    mat2.setToMirroring(AcGePlane::kXYPlane);
    pCopy->transformBy(mat2);
    return pCopy;
}

AcArray<AcDbEntity*> KTArxEntity::MirrorEnt(const AcDbObjectIdArray& entityIds, const AcGePoint3d& point1, const AcGePoint3d& point2) {
    AcArray<AcDbEntity*> arrpEnt;
    AcGeMatrix3d mat;
    mat.setToMirroring(AcGeLine3d(point1, point2));

    for (int i = 0; i < entityIds.length(); i++) {
        AcDbEntityPointer entity(entityIds[i], AcDb::kForRead);
        if (Acad::eOk != entity.openStatus()) {
            continue;
        }
        AcDbEntity* pCopy = AcDbEntity::cast(entity->clone());
        if (pCopy == nullptr) {
            continue;
        }
        pCopy->transformBy(mat);
        arrpEnt.append(pCopy);
    }

    return arrpEnt;
}

AcArray<AcDbEntity*> KTArxEntity::MirrorEnt(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& point1, const AcGePoint3d& point2) {
    AcArray<AcDbEntity*> arrpEnt;
    AcGeMatrix3d mat;
    mat.setToMirroring(AcGeLine3d(point1, point2));

    for (int i = 0; i < entities.length(); i++) {
        AcDbEntity* const source = entities[i];
        if (source == nullptr) {
            continue;
        }
        AcDbEntity* pCopy = AcDbEntity::cast(source->clone());
        if (pCopy == nullptr) {
            continue;
        }
        pCopy->transformBy(mat);
        arrpEnt.append(pCopy);
    }

    return arrpEnt;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
AcArray<AcDbEntity*> KTArxEntity::AnnularMatrix(const AcDbObjectIdArray& entityIds, const AcGePoint3d& basePoint, int currentCount, int totalCount) {
    AcArray<AcDbEntity*> arrpMatrix;

    if (totalCount < 2 || entityIds.isEmpty()) {
        return arrpMatrix;
    }

    AcArray<double> arrAngle;
    constexpr double kFullCircleDegrees = 360.0;
    const double angleStepDegrees = kFullCircleDegrees / static_cast<double>(totalCount);
    for (int i = 1; i < currentCount; i++) {
        arrAngle.append(angleStepDegrees * static_cast<double>(i));
    }

    for (int i = 0; i < arrAngle.length(); i++) {
        AcArray<AcDbEntity*> arrpCopy = CopyEnt(entityIds);
        RotateEnt(arrpCopy, basePoint, KTArxConvert::ToRadian(arrAngle[i]));
        arrpMatrix.append(arrpCopy);
    }

    return arrpMatrix;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
AcArray<AcDbEntity*> KTArxEntity::AnnularMatrix(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& basePoint, int currentCount, int totalCount) {
    AcArray<AcDbEntity*> arrpMatrix;

    if (currentCount < 2 || entities.isEmpty() || totalCount < 1) {
        return arrpMatrix;
    }

    AcArray<double> arrAngle;
    constexpr double kFullCircleDegrees = 360.0;
    const double angleStepDegrees = kFullCircleDegrees / static_cast<double>(totalCount);
    for (int i = 1; i < currentCount; i++) {
        arrAngle.append(angleStepDegrees * static_cast<double>(i));
    }

    for (int i = 0; i < arrAngle.length(); i++) {
        AcArray<AcDbEntity*> arrpCopy = CopyEnt(entities);
        RotateEnt(arrpCopy, basePoint, KTArxConvert::ToRadian(arrAngle[i]));
        arrpMatrix.append(arrpCopy);
    }

    return arrpMatrix;
}

bool KTArxEntity::IsClosedPline(AcDbPolyline* polyline) {
    if (polyline == nullptr) {
        return false;
    }
    if (polyline->isClosed()) {
        return true;
    }
    AcGePoint3d ptStart, ptEnd;
    polyline->getStartPoint(ptStart);
    polyline->getEndPoint(ptEnd);
    return ptStart.isEqualTo(ptEnd);
}

void KTArxEntity::SetEntSelected(const AcDbObjectIdArray& entityIds, bool selected /*= true*/, bool highlight /*= false*/) {
    //	acedSSDel(ent, ssname);		//将实体从集合中删除掉
    acedSSSetFirst(nullptr, nullptr);  // 将选中状态清除
    if (selected) {
        // 使实体被选中
        std::array<std::remove_extent_t<ads_name>, std::extent_v<ads_name>> selectionSet{};
        ads_name entityName{};

        acdbNameClear(selectionSet);
        for (int i = 0; i < entityIds.length(); ++i) {
            acdbGetAdsName(entityName, entityIds.at(i));
            acedSSAdd(&entityName[0], selectionSet.data(), selectionSet.data());  // 将实体增加到集合中
        }
        acedSSSetFirst(selectionSet.data(), nullptr);  // 把实体变为选中状态
    }

    if (highlight) {
        for (int i = 0; i < entityIds.length(); ++i) {
            AcDbEntityPointer entity(entityIds.at(i), AcDb::kForWrite);
            if (entity.openStatus() != Acad::eOk) {
                continue;
            }
            entity->highlight();  // 设置实体为高亮状态
        }
    }
}

bool KTArxEntity::SetEntToBottom(const AcDbObjectId& entityId, AcDbDatabase* database /*= acdbCurDwg()*/) {
    AcDbObjectId spaceId = database->currentSpaceId();
    AcDbObjectPointer<AcDbBlockTableRecord> pBTR(spaceId, AcDb::kForRead);
    if (pBTR.openStatus() != Acad::eOk) {
        return false;
    }
    AcDbSortentsTable* pSortTab = nullptr;
    AcDbObjectIdArray idarrTemp;
    idarrTemp.append(entityId);
    const Acad::ErrorStatus errorStatus = pBTR->getSortentsTable(pSortTab, AcDb::kForWrite, true);
    if (Acad::eOk != errorStatus) {
        return false;
    }
    pSortTab->moveToBottom(idarrTemp);
    pSortTab->close();
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool KTArxEntity::StretchEnt(const AcDbObjectIdArray& entityIds, const AcGePoint3d& cornerPoint1, const AcGePoint3d& cornerPoint2, const AcGePoint3d& basePoint, const AcGePoint3d& targetPoint) {
    AcGeVector3d stretchVec = targetPoint - basePoint;  // 拉伸距离
    AcGeBoundBlock3d box;
    const AcGeVector3d cornerVector{cornerPoint2 - cornerPoint1};
    box.set(cornerPoint1, AcGeVector3d(cornerVector.x, 0, 0), AcGeVector3d(0, cornerVector.y, 0), AcGeVector3d(0, 0, cornerVector.z));

    for (int i = 0; i < entityIds.length(); ++i) {
        AcDbObjectPointer<AcDbEntity> entity(entityIds[i], AcDb::kForWrite);
        if (Acad::eOk != entity.openStatus()) {
            continue;
        }
        AcGePoint3dArray arrpt;
        if (Acad::eOk != entity->getStretchPoints(arrpt)) {
            continue;
        }
        AcDbIntArray arrint;
        bool bFound = false;
        for (int j = 0; j < arrpt.length(); ++j) {
            const AcGePoint3d& currentPoint = arrpt[j];
            if (box.contains(currentPoint)) {
                arrint.append(j);
                bFound = true;
            }
        }
        if (bFound) {
            entity->moveStretchPointsAt(arrint, stretchVec);
        }
    }
    return true;
}

AcDbObjectIdArray KTArxEntity::GetEntIdByPt(const AcGePoint3d& point, bool all /*= false*/) {
    AcDbObjectIdArray entityIds;

    std::array<std::remove_extent_t<ads_name>, std::extent_v<ads_name>> selectionSet{};

    if (all) {
        // 得到通过该点的所有实体
        if (RTNORM != acedSSGet(_T("C"), asDblArray(point), asDblArray(point), nullptr, selectionSet.data())) {
            return entityIds;
        }
    } else {
        // 得到位于该点最上面的实体
        if (RTNORM != acedSSGet(nullptr, asDblArray(point), nullptr, nullptr, selectionSet.data())) {
            return entityIds;
        }
    }
    // 遍历选择集
    int selectionLength = 0;
    acedSSLength(selectionSet.data(), &selectionLength);
    for (int i = 0; i < selectionLength; i++) {
        std::array<std::remove_extent_t<ads_name>, std::extent_v<ads_name>> entityName{};
        acedSSName(selectionSet.data(), i, entityName.data());

        AcDbObjectId objectId;
        acdbGetObjectId(objectId, entityName.data());
        entityIds.append(objectId);
    }
    acedSSFree(selectionSet.data());

    return entityIds;
}

AcDbExtents KTArxEntity::GetEntExtents(const AcDbObjectId& entityId) {
    AcDbExtents extents;
    AcDbEntityPointer entity(entityId, AcDb::kForRead);
    if (Acad::eOk != entity.openStatus()) {
        return extents;
    }
    entity->getGeomExtents(extents);
    return extents;
}

AcDbExtents KTArxEntity::GetEntExtents(const AcArray<AcDbEntity*>& entities) {
    AcDbExtents extents;
    for (int i = 0; i < entities.length(); i++) {
        AcDbEntity* const entity = entities[i];
        if (entity == nullptr) {
            continue;
        }
        AcDbExtents oneExtents;
        entity->getGeomExtents(oneExtents);
        extents.addExt(oneExtents);
    }
    return extents;
}

AcDbExtents KTArxEntity::GetEntExtents(const AcDbObjectIdArray& entityIds) {
    AcDbExtents extents;
    for (int i = 0; i < entityIds.length(); i++) {
        AcDbEntityPointer entity(entityIds[i], AcDb::kForRead);
        if (Acad::eOk != entity.openStatus()) {
            continue;
        }
        AcDbExtents oneExtents;
        entity->getGeomExtents(oneExtents);
        extents.addExt(oneExtents);
    }
    return extents;
}

bool KTArxEntity::GetExtentsPt(const AcDbObjectId& entityId, AcGePoint3d& maxPoint, AcGePoint3d& minPoint) {
    AcDbEntityPointer entity(entityId, AcDb::kForRead);
    if (Acad::eOk != entity.openStatus()) {
        return false;
    }
    AcDbExtents extents;
    entity->getGeomExtents(extents);
    maxPoint = extents.maxPoint();
    minPoint = extents.minPoint();
    return true;
}

bool KTArxEntity::GetExtentsPt(const AcDbObjectIdArray& entityIds, AcGePoint3d& maxPoint, AcGePoint3d& minPoint) {
    AcDbExtents extents;
    for (int i = 0; i < entityIds.length(); i++) {
        AcDbEntityPointer entity(entityIds[i], AcDb::kForRead);
        if (Acad::eOk != entity.openStatus()) {
            continue;
        }
        AcDbExtents oneExtents;
        entity->getGeomExtents(oneExtents);
        extents.addExt(oneExtents);
    }
    maxPoint = extents.maxPoint();
    minPoint = extents.minPoint();
    return true;
}

bool KTArxEntity::GetExtentsPt(AcDbEntity* entity, AcGePoint3d& maxPoint, AcGePoint3d& minPoint) {
    if (entity == nullptr) {
        return false;
    }
    AcDbExtents extents;
    const Acad::ErrorStatus errorStatus = entity->getGeomExtents(extents);
    if (Acad::eOk != errorStatus) {
        return false;
    }
    maxPoint = extents.maxPoint();
    minPoint = extents.minPoint();
    return true;
}
}  // namespace KTArxTool