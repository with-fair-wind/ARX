#include <Test/test1.h>

namespace Test
{
    void test1()
    {
        AcDbLine *pline = new AcDbLine(AcGePoint3d::kOrigin, {100, 100, 0});
        KK::KKArxUtility utility;
        utility.postToModelSpace(pline);
    }

} // namespace Test
