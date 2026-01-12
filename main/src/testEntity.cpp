#include <Entity/testEntity.h>

#include <array>
#include <cmath>

namespace Test_Entity {
ACRX_DXF_DEFINE_MEMBERS(TestEntity, AcDbEntity, AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent, AcDbProxyEntity::kAllAllowedBits, TESTENTITY, main)
ACRX_DXF_DEFINE_MEMBERS(TestEntity2, AcDbEntity, AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent, AcDbProxyEntity::kAllAllowedBits, TESTENTITY2, main)

Adesk::Boolean TestEntity::subWorldDraw(AcGiWorldDraw* worldDraw) {
    (void)worldDraw;
    return Adesk::kFalse;
}

Adesk::UInt32 TestEntity::subViewportDrawLogicalFlags(AcGiViewportDraw* viewportDraw) {
    (void)viewportDraw;
    return kDrawableViewDependentViewportDraw;
}

void TestEntity::subViewportDraw(AcGiViewportDraw* viewportDraw) {
    constexpr double kViewDirDotThreshold = 0.9;
    constexpr double kBaseSize = 100.0;
    constexpr double kTopZ = 0.0;
    constexpr double kSideZ = 100.0;
    constexpr int kColorIndex = 1;
    constexpr std::size_t kPolylinePointCount = 5U;

    const AcGeVector3d viewDir = viewportDraw->viewport().viewDir();

    std::array<AcGePoint3d, kPolylinePointCount> dcPoints{};

    // Top view
    if (std::abs(viewDir.dotProduct(AcGeVector3d::kZAxis)) > kViewDirDotThreshold && viewDir.z > 0.0) {
        dcPoints[0] = AcGePoint3d{0.0, 0.0, kTopZ};
        dcPoints[1] = AcGePoint3d{kBaseSize, 0.0, kTopZ};
        dcPoints[2] = AcGePoint3d{kBaseSize, kBaseSize, kTopZ};
        dcPoints[3] = AcGePoint3d{0.0, kBaseSize, kTopZ};
    }
    // Left view
    else if (std::abs(viewDir.dotProduct(AcGeVector3d::kXAxis)) > kViewDirDotThreshold && viewDir.x < 0.0) {
        dcPoints[0] = AcGePoint3d{0.0, 0.0, 0.0};
        dcPoints[1] = AcGePoint3d{0.0, 0.0, kSideZ};
        dcPoints[2] = AcGePoint3d{kBaseSize, 0.0, kSideZ};
        dcPoints[3] = AcGePoint3d{kBaseSize, 0.0, 0.0};
    } else {
        return;  // 其他视角不绘制
    }

    dcPoints[4] = dcPoints[0];

    viewportDraw->subEntityTraits().setColor(kColorIndex);
    viewportDraw->geometry().polylineDc(static_cast<int>(dcPoints.size()), dcPoints.data());
}

Adesk::Boolean TestEntity2::subWorldDraw(AcGiWorldDraw* mode) {
    assertReadEnabled();
    mode->geometry().circle(m_centerPos, m_radius, AcGeVector3d::kZAxis);
    return Adesk::kTrue;
}

Acad::ErrorStatus TestEntity2::dwgInFields(AcDbDwgFiler* filer) {
    assertWriteEnabled();
    if (Acad::ErrorStatus errorStatus = AcDbEntity::dwgInFields(filer); errorStatus != Acad::eOk) return errorStatus;
    filer->readPoint3d(&m_centerPos);
    filer->readDouble(&m_radius);
    filer->readInt32(&m_testNum);
    return Acad::eOk;
}

Acad::ErrorStatus TestEntity2::dwgOutFields(AcDbDwgFiler* filer) const {
    assertReadEnabled();
    if (Acad::ErrorStatus errorStatus = AcDbEntity::dwgOutFields(filer); errorStatus != Acad::eOk) return errorStatus;
    filer->writePoint3d(m_centerPos);
    filer->writeDouble(m_radius);
    filer->writeInt32(m_testNum);
    return Acad::eOk;
}
}  // namespace Test_Entity
