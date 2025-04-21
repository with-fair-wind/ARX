#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxConvert;
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

        /// @brief ����ʵ��
        AcDbEntity *MirrorEnt(AcDbEntity *pEnt, const AcGePoint3d &pt1, const AcGePoint3d &pt2);
        AcArray<AcDbEntity *> MirrorEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &pt1, const AcGePoint3d &pt2);
        AcArray<AcDbEntity *> MirrorEnt(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &pt1, const AcGePoint3d &pt2);

        /// @brief ���ξ���
        /// @param nCurNum ��ǰ��Ҫ������
        /// @param nSum �ܹ�������
        AcArray<AcDbEntity *> AnnularMatrix(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, int nCurNum, int nSum);
        AcArray<AcDbEntity *> AnnularMatrix(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &ptBase, int nCurNum, int nSum);

        /// @brief ����ʵ�屻ѡ��
        void SetEntSelected(const AcDbObjectIdArray &arridEnt, bool bSeled = true, bool bHighlight = false);

        /// @brief ������ͼ����
        bool SetEntToBottom(const AcDbObjectId &id, AcDbDatabase *pDb = acdbCurDwg());

        /// @brief ����ʵ��
        /// @param arrid        ������Ҫ�������ʵ��id����
        /// @param ptCorner1    ���뱻����Ľǵ�1
        /// @param ptCorner2    ���뱻����Ľǵ�2
        /// @param ptBase       �������
        /// @param ptTarget     ����Ŀ���
        /// @return
        bool StretchEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptCorner1, const AcGePoint3d &ptCorner2,
                        const AcGePoint3d &ptBase, const AcGePoint3d &ptTarget);

        /// @brief �õ������������ϵ�ʵ��id
        AcDbObjectIdArray GetEntIdByPt(const AcGePoint3d &pt, bool bAll = false);

        /// @brief �õ�ʵ���������
        AcDbExtents GetEntExtents(const AcDbObjectId &idEnt);
        AcDbExtents GetEntExtents(const AcDbObjectIdArray &arrid);
        AcDbExtents GetEntExtents(const AcArray<AcDbEntity *> &arrEnt);

        /// @brief �õ�ʵ��������ε��������С��
        bool GetExtentsPt(const AcDbObjectId &idEnt, AcGePoint3d &ptMax, AcGePoint3d &ptMin);
        bool GetExtentsPt(AcDbEntity *pEnt, AcGePoint3d &ptMax, AcGePoint3d &ptMin);
        bool GetExtentsPt(const AcDbObjectIdArray &arrid, AcGePoint3d &ptMax, AcGePoint3d &ptMin);

#pragma region polyline
        /// @brief �жϸö�����Ƿ��Ǳպ϶����
        bool IsClosedPline(AcDbPolyline *pPline);
#pragma endregion

    private:
        std::shared_ptr<KTArxConvert> m_pArxConvert;
    };
} // namespace KTArxTool
