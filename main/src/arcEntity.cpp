#include <Entity/arcEntity.h>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = kPi * 2.0;
constexpr double kHalf = 0.5;

AcGePoint3d arcMidPoint(const AcGeCircArc3d& arc) {
    double midAng = (arc.startAng() + arc.endAng()) * kHalf;
    if (arc.endAng() < arc.startAng()) {
        midAng += kPi;
    }
    return arc.evalPoint(midAng);
}
}  // namespace

ACRX_DXF_DEFINE_MEMBERS(
    ArcEntity, AcDbEntity,
    AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent,
    AcDbProxyEntity::kAllAllowedBits,
    ARCENTITY, main)

const AcGeCircArc3d& ArcEntity::arc() const {
    assertReadEnabled();
    return m_arc;
}

void ArcEntity::setArc(const AcGeCircArc3d& arc) {
    assertWriteEnabled();
    m_arc = arc;
}

// ─── 序列化 ───

Acad::ErrorStatus ArcEntity::dwgOutFields(AcDbDwgFiler* filer) const {
    assertReadEnabled();
    Acad::ErrorStatus status = AcDbEntity::dwgOutFields(filer);
    if (status != Acad::eOk) {
        return status;
    }

    filer->writeInt16(kCurrentVersion);
    filer->writePoint3d(m_arc.center());
    filer->writeVector3d(m_arc.normal());
    filer->writeVector3d(m_arc.refVec());
    filer->writeDouble(m_arc.radius());
    filer->writeDouble(m_arc.startAng());
    filer->writeDouble(m_arc.endAng());

    return filer->filerStatus();
}

Acad::ErrorStatus ArcEntity::dwgInFields(AcDbDwgFiler* filer) {
    assertWriteEnabled();
    Acad::ErrorStatus status = AcDbEntity::dwgInFields(filer);
    if (status != Acad::eOk) {
        return status;
    }

    Adesk::Int16 version = 0;
    filer->readInt16(&version);
    if (version < 1) {
        return Acad::eMakeMeProxy;
    }

    AcGePoint3d center;
    AcGeVector3d normal;
    AcGeVector3d refVec;
    double radius = 0.0;
    double startAng = 0.0;
    double endAng = 0.0;

    filer->readPoint3d(&center);
    filer->readVector3d(&normal);
    filer->readVector3d(&refVec);
    filer->readDouble(&radius);
    filer->readDouble(&startAng);
    filer->readDouble(&endAng);

    if (filer->filerStatus() != Acad::eOk) {
        return filer->filerStatus();
    }

    m_arc.set(center, normal, refVec, radius, startAng, endAng);
    return Acad::eOk;
}

// ─── 绘制 ───

Adesk::Boolean ArcEntity::subWorldDraw(AcGiWorldDraw* mode) {
    assertReadEnabled();

    AcGeVector3d startVec = m_arc.refVec().rotateBy(m_arc.startAng(), m_arc.normal());
    double sweepAngle = m_arc.endAng() - m_arc.startAng();
    if (sweepAngle <= 0.0) {
        sweepAngle += kTwoPi;
    }

    mode->geometry().circularArc(
        m_arc.center(),
        m_arc.radius(),
        m_arc.normal(),
        startVec,
        sweepAngle);

    return Adesk::kTrue;
}

// ─── 夹点 ───

Acad::ErrorStatus ArcEntity::subGetGripPoints(
    AcGePoint3dArray& gripPoints,
    AcDbIntArray& /*osnapModes*/,
    AcDbIntArray& /*geomIds*/) const
{
    assertReadEnabled();

    gripPoints.append(m_arc.startPoint());    // grip 0: 起点
    gripPoints.append(m_arc.endPoint());      // grip 1: 终点
    gripPoints.append(arcMidPoint(m_arc));    // grip 2: 中点（改凸度）
    gripPoints.append(m_arc.center());        // grip 3: 圆心（整体移动）

    return Acad::eOk;
}

Acad::ErrorStatus ArcEntity::subMoveGripPointsAt(
    const AcDbIntArray& indices,
    const AcGeVector3d& offset)
{
    assertWriteEnabled();
    AcGeCircArc3d snapshot = m_arc;

    AcGePoint3d ptStart = m_arc.startPoint();
    AcGePoint3d ptEnd = m_arc.endPoint();
    AcGePoint3d ptMid = arcMidPoint(m_arc);

    for (int i = 0; i < indices.length(); ++i) {
        switch (indices[i]) {
            case 0: ptStart += offset; break;
            case 1: ptEnd += offset; break;
            case 2: ptMid += offset; break;
            case 3: {
                AcGeMatrix3d mat;
                mat.setToTranslation(offset);
                m_arc.transformBy(mat);
                return Acad::eOk;
            }
            default:
                break;
        }
    }

    AcGeCircArc3d newArc;
    AcGeError err = AcGe::kOk;
    newArc.set(ptStart, ptMid, ptEnd, err);
    if (err != AcGe::kOk) {
        m_arc = snapshot;
        return Acad::eInvalidInput;
    }
    m_arc = newArc;

    return Acad::eOk;
}

// ─── 变换 ───

Acad::ErrorStatus ArcEntity::subTransformBy(const AcGeMatrix3d& xform) {
    assertWriteEnabled();
    m_arc.transformBy(xform);
    return Acad::eOk;
}
