#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxConvert {
   public:
    // 功能：转为三维点
    static AcGePoint3d ToPt3d(const AcGePoint2d& point, double zCoord = 0.0) { return AcGePoint3d(point.x, point.y, zCoord); }

    // 功能：角度转换为弧度
    static double ToRadian(double angleDegrees) {
        static constexpr double kDegreesToRadians = (M_PI / 180.0);
        return angleDegrees * kDegreesToRadians;
    }
};
}  // namespace KTArxTool
