#include <Test/command.h>
#include <Entity/testEntity.h>

namespace Test_Command
{
    void test1()
    {
        AcDbLine *pline = new AcDbLine(AcGePoint3d::kOrigin, {100, 100, 0});
        KTArxTool::KTArxUtility utility;
        utility.PostToModelSpace(pline);
    }

    void CreatePolylineWithBulge(AcDbPolyline *pline, const AcGePoint2dArray &pts, const double *bulgeArray)
    {
        for (int i = 0; i < pts.length(); i++)
            pline->addVertexAt(i, pts[i], bulgeArray[i], 0, 0);
    }

    void test2()
    {
        AcDbPolyline *pPline = new AcDbPolyline(); // Ĭ�϶�����Ϊ0

        AcGePoint2dArray points;
        points.append(AcGePoint2d(0, 0));
        points.append(AcGePoint2d(100, 0));
        points.append(AcGePoint2d(100, 100));

        // double bulges[] = {0.0, 1.0, -0.5}; // �ڶ���Ϊ��Բ��������Ϊ˳ʱ���ķ�֮һԲ
        // double bulges[] = {0.0, 1.0, 0.0}; // �ڶ���Ϊ��Բ��������Ϊ˳ʱ���ķ�֮һԲ
        double bulge = tan((-90.0 / 4) * M_PI / 180.0);
        double bulges[] = {0.0, 1.0, bulge}; // �ڶ���Ϊ��Բ��������Ϊ˳ʱ���ķ�֮һԲ
        CreatePolylineWithBulge(pPline, points, bulges);

        pPline->setClosed(true);
        KTArxTool::KTArxUtility utility;
        utility.PostToModelSpace(pPline);
    }

    void test3()
    {
        using namespace Test_Entity;
        TestEntity *pEnt = new TestEntity();
        KTArxTool::KTArxUtility utility;
        utility.PostToModelSpace(pEnt);
    }

    void test4()
    {
        using namespace Test_Entity;
        TestEntity2 *pEnt = new TestEntity2{};
        KTArxTool::KTArxUtility utility;
        AcDbObjectId entId = utility.PostToModelSpace(pEnt);
    }

    void test5()
    {
        using namespace Test_Entity;
        KTArxTool::KTArxUiPr uiPr;
        AcDbObjectId entId;
        AcGePoint3d pt;
        if (uiPr.SelEnt(_T("ѡ��ʵ��"), TestEntity2::desc(), entId, pt))
        {
            if (entId.isValid())
            {
                AcDbObjectPointer<TestEntity2> pTestEnt(entId, AcDb::kForWrite);
                {
                    if (pTestEnt.openStatus() == Acad::eOk)
                    {
                        // pTestEnt->setCenterPos(AcGePoint3d(100, 100, 0));
                        pTestEnt->setTestNum(1000);
                    }
                }
            }
        }
    }
} // namespace Test_Command
