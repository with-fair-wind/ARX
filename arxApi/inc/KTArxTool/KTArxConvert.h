#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxConvert
    {
    public:
        // ���ܣ�תΪ��ά��
        AcGePoint3d ToPt3d(const AcGePoint2d &pt, double z = 0.) { return AcGePoint3d(pt.x, pt.y, z); }

        // ���ܣ��Ƕ�ת��Ϊ����
        double ToRadian(double dAngle) { return (dAngle * (M_PI / 180.0)); }
    };
} // namespace KTArxTool
