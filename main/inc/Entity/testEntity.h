#pragma once

namespace Test_Entity {
class TestEntity : public AcDbEntity {
   public:
    ACRX_DECLARE_MEMBERS(TestEntity)

    TestEntity() = default;
    virtual ~TestEntity() = default;

   protected:
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* mode) override;
    virtual void subViewportDraw(AcGiViewportDraw* mode) override;
    virtual Adesk::UInt32 subViewportDrawLogicalFlags(AcGiViewportDraw* vd) override;
};

class TestEntity2 : public AcDbEntity {
   public:
    ACRX_DECLARE_MEMBERS(TestEntity2)

    TestEntity2() = default;
    virtual ~TestEntity2() = default;

    PROPERTY_CREATE_ARX(TestEntity2, AcGePoint3d, CenterPos, m_centerPos)
    PROPERTY_CREATE_ARX(TestEntity2, double, Radius, m_radius)
    PROPERTY_CREATE_ARX(TestEntity2, int32_t, TestNum, m_testNum)

   protected:
    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;
    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw* mode) override;

   private:
    AcGePoint3d m_centerPos = AcGePoint3d::kOrigin;
    double m_radius = 100.0;
    int32_t m_testNum = 0;
};
}  // namespace Test_Entity
