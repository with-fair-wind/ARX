#pragma once

namespace Test_Entity {
class TestEntity : public AcDbEntity {
   public:
    ACRX_DECLARE_MEMBERS(TestEntity)

    TestEntity() = default;
    TestEntity(const TestEntity&) = delete;
    TestEntity& operator=(const TestEntity&) = delete;
    TestEntity(TestEntity&&) = delete;
    TestEntity& operator=(TestEntity&&) = delete;
    ~TestEntity() override = default;

   protected:
    Adesk::Boolean subWorldDraw(AcGiWorldDraw* worldDraw) override;
    void subViewportDraw(AcGiViewportDraw* viewportDraw) override;
    Adesk::UInt32 subViewportDrawLogicalFlags(AcGiViewportDraw* viewportDraw) override;
};

class TestEntity2 : public AcDbEntity {
   public:
    ACRX_DECLARE_MEMBERS(TestEntity2)

    TestEntity2() = default;
    TestEntity2(const TestEntity2&) = delete;
    TestEntity2& operator=(const TestEntity2&) = delete;
    TestEntity2(TestEntity2&&) = delete;
    TestEntity2& operator=(TestEntity2&&) = delete;
    ~TestEntity2() override = default;

    PROPERTY_CREATE_ARX(TestEntity2, AcGePoint3d, CenterPos, m_centerPos)
    PROPERTY_CREATE_ARX(TestEntity2, double, Radius, m_radius)
    PROPERTY_CREATE_ARX(TestEntity2, int32_t, TestNum, m_testNum)

   protected:
    Acad::ErrorStatus dwgInFields(AcDbDwgFiler* filer) override;
    Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* filer) const override;
    Adesk::Boolean subWorldDraw(AcGiWorldDraw* mode) override;

   private:
    AcGePoint3d m_centerPos = AcGePoint3d::kOrigin;
    double m_radius = 100.0;
    int32_t m_testNum = 0;
};
}  // namespace Test_Entity
