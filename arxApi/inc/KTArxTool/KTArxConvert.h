#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxConvert {
   public:
    // 功能：转为三维点
    AcGePoint3d ToPt3d(const AcGePoint2d& pt, double z = 0.) { return AcGePoint3d(pt.x, pt.y, z); }

    // 功能：角度转换为弧度
    double ToRadian(double dAngle) { return (dAngle * (M_PI / 180.0)); }
};
}  // namespace KTArxTool
