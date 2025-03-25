#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxEntity
    {
    public:
        /// @brief ����������ʵ��
        bool ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale, const AcGePoint3d &ptBase);

        /// @brief �ֽ�ʵ��
        AcArray<AcDbEntity *> ExplodeEnt(AcDbEntity *pEnt);
        AcArray<AcDbEntity *> ExplodeEnt(const AcDbObjectId &idEnt);

        /// @brief �ƶ�ʵ��
        bool MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);

        /// @brief ��תʵ��(����)
        bool RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation);
        bool RotateEnt(const AcArray<AcDbEntity *> &arrpEnt, const AcGePoint3d &ptBase, double rotation);

        /// @brief ɾ��ʵ��
        bool DeleteEnt(const AcDbObjectId &idEnt);
        void DeleteEnt(AcArray<AcDbEntity *> &arrpEnt);

        /// @brief ����ʵ��
        AcArray<AcDbEntity *> CopyEnt(const AcDbObjectIdArray &arrid);
        AcArray<AcDbEntity *> CopyEnt(const AcArray<AcDbEntity *> &arrp);
        AcDbEntity *CopyEnt(const AcDbObjectId &idEnt);

        // Summary:
        // Time:	  2020��3��5�� peihaodong
        // Explain:

        /// @brief ����ʵ��
        AcDbEntity *MirrorEnt(AcDbEntity *pEnt, const AcGePoint3d &pt1, const AcGePoint3d &pt2);
        AcArray<AcDbEntity *> MirrorEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &pt1, const AcGePoint3d &pt2);
        AcArray<AcDbEntity *> MirrorEnt(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &pt1, const AcGePoint3d &pt2);

        /// @brief ���ξ���
        /// @param nCurNum ��ǰ��Ҫ������
        /// @param nSum �ܹ�������
        AcArray<AcDbEntity *> AnnularMatrix(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, int nCurNum, int nSum);
        AcArray<AcDbEntity *> AnnularMatrix(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &ptBase, int nYxcs, int nSum);

#pragma region polyline
        /// @brief �жϸö�����Ƿ��Ǳպ϶����
        bool IsClosedPline(AcDbPolyline *pPline);
#pragma endregion
    };
} // namespace KTArxTool
