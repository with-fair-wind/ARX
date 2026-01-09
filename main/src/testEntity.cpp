#include <Entity/testEntity.h>

namespace Test_Entity {
ACRX_DXF_DEFINE_MEMBERS(TestEntity, AcDbEntity, AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent, AcDbProxyEntity::kAllAllowedBits, TESTENTITY, main)
ACRX_DXF_DEFINE_MEMBERS(TestEntity2, AcDbEntity, AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent, AcDbProxyEntity::kAllAllowedBits, TESTENTITY2, main)

Adesk::Boolean TestEntity::subWorldDraw(AcGiWorldDraw* mode) { return Adesk::kFalse; }

Adesk::UInt32 TestEntity::subViewportDrawLogicalFlags(AcGiViewportDraw* vd) { return kDrawableViewDependentViewportDraw; }

void TestEntity::subViewportDraw(AcGiViewportDraw* vd) {
#if 0
        AcGiViewport &vp = vd->viewport();
        AcGeVector3d dir = vp.viewDir();
        dir.normalize();
        AcGePoint2d ll,
            ur;
        vp.getViewportDcCorners(ll, ur);

        AcGePoint3d pts[5];
        double w = ur.x - ll.x, h = ur.y - ll.y;

        if (std::abs(dir.dotProduct(AcGeVector3d::kZAxis)) > 0.9)
        {
            // 俯视
            pts[0] = {ll.x + w * 0.1, ll.y + h * 0.1, 0};
            pts[1] = {ll.x + w * 0.9, ll.y + h * 0.1, 0};
            pts[2] = {ll.x + w * 0.9, ll.y + h * 0.9, 0};
            pts[3] = {ll.x + w * 0.1, ll.y + h * 0.9, 0};
        }
        else if (std::abs(dir.dotProduct(AcGeVector3d::kXAxis)) > 0.9 && dir.x < 0)
        {
            // 左视
            pts[0] = {ll.x + w * 0.1, ll.y + h * 0.1, 0};
            pts[1] = {ll.x + w * 0.1, ll.y + h * 0.9, 0};
            pts[2] = {ll.x + w * 0.9, ll.y + h * 0.9, 0};
            pts[3] = {ll.x + w * 0.9, ll.y + h * 0.1, 0};
        }
        else
        {
            return;
        }
        pts[4] = pts[0];
        vd->subEntityTraits().setColor(1);
        vd->geometry().polylineDc(5, pts);
#else
    auto& vpGeom = vd->geometry();
    auto dir = vd->viewport().viewDir();
    std::vector<AcGePoint3d> ptVec{{0, 0, 0}, {100, 0, 0}, {100, 100, 0}, {0, 100, 0}};  // 四个点
    AcGePoint3d dcPts[5];

    // Top view
    if (std::abs(dir.dotProduct(AcGeVector3d::kZAxis)) > 0.9 && dir.z > 0) {
        for (int i = 0; i < 4; i++)
            // vpGeom.worldToDc(ptVec[i], dcPts[i]);
            dcPts[i] = ptVec[i];
    }
    // Left view
    else if (std::abs(dir.dotProduct(AcGeVector3d::kXAxis)) > 0.9 && dir.x < 0) {
        AcGePoint3d bl = ptVec[0], tl = bl;
        tl.z = 100;
        AcGePoint3d br = ptVec[1], tr = br;
        tr.z = 100;
        // vpGeom.worldToDc(bl, dcPts[0]);
        // vpGeom.worldToDc(tl, dcPts[1]);
        // vpGeom.worldToDc(tr, dcPts[2]);
        // vpGeom.worldToDc(br, dcPts[3]);
        dcPts[0] = bl;
        dcPts[1] = tl;
        dcPts[2] = tr;
        dcPts[3] = br;
    } else
        return;  // 其他视角不绘制

    dcPts[4] = dcPts[0];
    vd->subEntityTraits().setColor(1);
    vd->geometry().polylineDc(5, dcPts);
#endif
}

Adesk::Boolean TestEntity2::subWorldDraw(AcGiWorldDraw* mode) {
    assertReadEnabled();
    mode->geometry().circle(m_centerPos, m_radius, AcGeVector3d::kZAxis);
    return Adesk::kTrue;
}

Acad::ErrorStatus TestEntity2::dwgInFields(AcDbDwgFiler* filer) {
    assertWriteEnabled();
    if (Acad::ErrorStatus es = AcDbEntity::dwgInFields(filer); es != Acad::eOk) return es;
    filer->readPoint3d(&m_centerPos);
    filer->readDouble(&m_radius);
    filer->readInt32(&m_testNum);
    return Acad::eOk;
}

Acad::ErrorStatus TestEntity2::dwgOutFields(AcDbDwgFiler* filer) const {
    assertReadEnabled();
    if (Acad::ErrorStatus es = AcDbEntity::dwgOutFields(filer); es != Acad::eOk) return es;
    filer->writePoint3d(m_centerPos);
    filer->writeDouble(m_radius);
    filer->writeInt32(m_testNum);
    return Acad::eOk;
}
}  // namespace Test_Entity
