#include <Entity/testEntity.h>
#include <KTArxTool/KTArxCompare.h>
#include <KTArxTool/KTArxConvert.h>
#include <KTArxTool/KTArxCurve.h>
#include <KTArxTool/KTArxEntity.h>
#include <KTArxTool/KTArxUiPr.h>
#include <KTArxTool/KTArxUtility.h>
#include <Test/command.h>

#include <cmath>
#include <memory>

namespace {
constexpr double kBaseSize = 100.0;
constexpr double kDefaultZ = 0.0;
constexpr double kQuarterCircleDegrees = -90.0;
constexpr double kBulgeDivisor = 4.0;
constexpr int32_t kUpdatedTestNum = 1000;

void CreatePolylineWithBulge(AcDbPolyline* polyline, const AcGePoint2dArray& points, const AcArray<double>& bulgeValues) {
    for (int i = 0; i < points.length(); i++) {
        polyline->addVertexAt(static_cast<unsigned int>(i), points.at(i), bulgeValues.at(i), 0, 0);
    }
}
}  // namespace

namespace Test_Command {
void test1() {
    const AcGePoint3d endPoint{kBaseSize, kBaseSize, kDefaultZ};
    auto line = std::make_unique<AcDbLine>(AcGePoint3d::kOrigin, endPoint);
    (void)KTArxTool::KTArxUtility::PostToModelSpace(line.release());
}

void test2() {
    auto polyline = std::make_unique<AcDbPolyline>();  // 默认顶点数为0

    AcGePoint2dArray points;
    points.append(AcGePoint2d(0.0, 0.0));
    points.append(AcGePoint2d(kBaseSize, 0.0));
    points.append(AcGePoint2d(kBaseSize, kBaseSize));

    const double bulge = std::tan(KTArxTool::KTArxConvert::ToRadian(kQuarterCircleDegrees / kBulgeDivisor));

    AcArray<double> bulges;
    bulges.append(0.0);
    bulges.append(1.0);
    bulges.append(bulge);  // 第三段为顺时针四分之一圆

    CreatePolylineWithBulge(polyline.get(), points, bulges);

    polyline->setClosed(true);
    (void)KTArxTool::KTArxUtility::PostToModelSpace(polyline.release());
}

void test3() {
    auto entity = std::make_unique<Test_Entity::TestEntity>();
    (void)KTArxTool::KTArxUtility::PostToModelSpace(entity.release());
}

void test4() {
    auto entity = std::make_unique<Test_Entity::TestEntity2>();
    (void)KTArxTool::KTArxUtility::PostToModelSpace(entity.release());
}

void test5() {
    AcDbObjectId entId;
    AcGePoint3d pickedPoint;
    if (KTArxTool::KTArxUiPr::SelEnt(_T("选择实体"), Test_Entity::TestEntity2::desc(), entId, pickedPoint)) {
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
}  // namespace Test_Command
