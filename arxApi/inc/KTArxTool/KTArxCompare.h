#pragma once

namespace KTArxCompare {
class KT_EXPORTARX_API KTArxCompare {
   public:
    /// @brief 比较两个点的大小
    bool isPtEqual(const AcGePoint3d& pt1, const AcGePoint3d& pt2, double dTol = 1e-6);
};
}  // namespace KTArxCompare
