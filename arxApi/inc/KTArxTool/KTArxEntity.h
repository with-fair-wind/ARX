#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxEntity
    {
    public:
        // Summary:   ����������ʵ��
        bool ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale, const AcGePoint3d &ptBase);

        // Summary:   �ֽ�ʵ��
        AcArray<AcDbEntity *> ExplodeEnt(AcDbEntity *pEnt);
        AcArray<AcDbEntity *> ExplodeEnt(const AcDbObjectId &idEnt);

        // Summary:   �ƶ�ʵ��
        bool MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);

        // Summary:   ��תʵ��(����)
        bool RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation);
        bool RotateEnt(const AcArray<AcDbEntity *> &arrpEnt, const AcGePoint3d &ptBase, double rotation);

#pragma region polyline
        /// @brief �жϸö�����Ƿ��Ǳպ϶����
        bool IsClosedPline(AcDbPolyline *pPline);
#pragma endregion
    };
} // namespace KTArxTool
