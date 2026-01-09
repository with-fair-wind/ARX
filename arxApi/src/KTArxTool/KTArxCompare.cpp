#include <KTArxTool/KTArxCompare.h>

namespace KTArxCompare {
bool KTArxCompare::isPtEqual(const AcGePoint3d& pt1, const AcGePoint3d& pt2, double dTol) {
    AcGeTol tol;
    tol.setEqualPoint(dTol);
    return pt1.isEqualTo(pt2, tol);
}
}  // namespace KTArxCompare
