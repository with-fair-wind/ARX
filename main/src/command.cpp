#include <Test/command.h>

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
        AcDbPolyline *pPline = new AcDbPolyline(); // 默认顶点数为0

        AcGePoint2dArray points;
        points.append(AcGePoint2d(0, 0));
        points.append(AcGePoint2d(100, 0));
        points.append(AcGePoint2d(100, 100));

        // double bulges[] = {0.0, 1.0, -0.5}; // 第二段为半圆，第三段为顺时针四分之一圆
        // double bulges[] = {0.0, 1.0, 0.0}; // 第二段为半圆，第三段为顺时针四分之一圆
        double bulge = tan((-90.0 / 4) * M_PI / 180.0);
        double bulges[] = {0.0, 1.0, bulge}; // 第二段为半圆，第三段为顺时针四分之一圆
        CreatePolylineWithBulge(pPline, points, bulges);

        pPline->setClosed(true);
        KTArxTool::KTArxUtility utility;
        utility.PostToModelSpace(pPline);
    }

} // namespace Test_Command
