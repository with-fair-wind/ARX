#pragma once

class ArcEntity : public AcDbEntity {
   public:
    ACRX_DECLARE_MEMBERS(ArcEntity)

    ArcEntity() = default;
    ~ArcEntity() override = default;

    const AcGeCircArc3d& arc() const;
    void setArc(const AcGeCircArc3d& arc);

   protected:
    Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;
    Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
    Adesk::Boolean subWorldDraw(AcGiWorldDraw* mode) override;

    Acad::ErrorStatus subGetGripPoints(
        AcGePoint3dArray& gripPoints,
        AcDbIntArray& osnapModes,
        AcDbIntArray& geomIds) const override;

    Acad::ErrorStatus subMoveGripPointsAt(
        const AcDbIntArray& indices,
        const AcGeVector3d& offset) override;

    Acad::ErrorStatus subTransformBy(const AcGeMatrix3d& xform) override;

   private:
    static constexpr Adesk::Int16 kCurrentVersion = 1;

    AcGeCircArc3d m_arc{
        AcGePoint3d::kOrigin,
        AcGeVector3d::kZAxis,
        AcGeVector3d::kXAxis,
        1.0,
        0.0,
        6.28318530717958647692  // 2*PI
    };
};
