#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxEntity
    {
    public:
        /// @brief 按比例缩放实体
        bool ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale, const AcGePoint3d &ptBase);

        /// @brief 分解实体
        AcArray<AcDbEntity *> ExplodeEnt(AcDbEntity *pEnt);
        AcArray<AcDbEntity *> ExplodeEnt(const AcDbObjectId &idEnt);

        /// @brief 移动实体
        bool MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);

        /// @brief 旋转实体(弧度)
        bool RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation);
        bool RotateEnt(const AcArray<AcDbEntity *> &arrpEnt, const AcGePoint3d &ptBase, double rotation);

        /// @brief 删除实体
        bool DeleteEnt(const AcDbObjectId &idEnt);
        void DeleteEnt(AcArray<AcDbEntity *> &arrpEnt);

        /// @brief 拷贝实体
        AcArray<AcDbEntity *> CopyEnt(const AcDbObjectIdArray &arrid);
        AcArray<AcDbEntity *> CopyEnt(const AcArray<AcDbEntity *> &arrp);
        AcDbEntity *CopyEnt(const AcDbObjectId &idEnt);

        // Summary:
        // Time:	  2020年3月5日 peihaodong
        // Explain:

        /// @brief 镜像实体
        AcDbEntity *MirrorEnt(AcDbEntity *pEnt, const AcGePoint3d &pt1, const AcGePoint3d &pt2);
        AcArray<AcDbEntity *> MirrorEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &pt1, const AcGePoint3d &pt2);
        AcArray<AcDbEntity *> MirrorEnt(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &pt1, const AcGePoint3d &pt2);

        /// @brief 环形矩阵
        /// @param nCurNum 当前需要的数量
        /// @param nSum 总共的数量
        AcArray<AcDbEntity *> AnnularMatrix(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, int nCurNum, int nSum);
        AcArray<AcDbEntity *> AnnularMatrix(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &ptBase, int nYxcs, int nSum);

#pragma region polyline
        /// @brief 判断该多段线是否是闭合多段线
        bool IsClosedPline(AcDbPolyline *pPline);
#pragma endregion
    };
} // namespace KTArxTool
