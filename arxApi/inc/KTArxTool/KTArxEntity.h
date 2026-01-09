#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxConvert;
class KT_EXPORTARX_API KTArxEntity {
   public:
    /// @brief 按比例缩放实体
    bool ModifyEntByScale(AcDbEntity* pEnt, double dScale, const AcGePoint3d& ptBase);
    bool ModifyEntByScale(const AcDbObjectId& idEnt, double dScale, const AcGePoint3d& ptBase);
    bool ModifyEntByScale(const AcArray<AcDbEntity*>& arrpEnt, double dScale, const AcGePoint3d& ptBase);

    /// @brief 分解实体
    AcArray<AcDbEntity*> ExplodeEnt(AcDbEntity* pEnt);
    AcArray<AcDbEntity*> ExplodeEnt(const AcDbObjectId& idEnt);

    /// @brief 移动实体
    bool MoveEnt(AcDbEntity* pEnt, const AcGePoint3d& ptBase, const AcGePoint3d& ptDest);
    bool MoveEnt(const AcDbObjectIdArray& arrid, const AcGePoint3d& ptBase, const AcGePoint3d& ptDest);
    bool MoveEnt(const AcDbObjectId& idEnt, const AcGePoint3d& ptBase, const AcGePoint3d& ptDest);
    bool MoveEnt(const AcArray<AcDbEntity*>& arrEnt, const AcGePoint3d& ptBase, const AcGePoint3d& ptDest);

    /// @brief 旋转实体(弧度)
    bool RotateEnt(AcDbEntity* pEnt, const AcGePoint3d& ptBase, double rotation);
    bool RotateEnt(const AcArray<AcDbEntity*>& arrpEnt, const AcGePoint3d& ptBase, double rotation);

    /// @brief 删除实体
    bool DeleteEnt(const AcDbObjectId& idEnt);
    void DeleteEnt(AcArray<AcDbEntity*>& arrpEnt);

    /// @brief 拷贝实体
    AcArray<AcDbEntity*> CopyEnt(const AcDbObjectIdArray& arrid);
    AcArray<AcDbEntity*> CopyEnt(const AcArray<AcDbEntity*>& arrp);
    AcDbEntity* CopyEnt(const AcDbObjectId& idEnt);

    /// @brief 镜像实体
    AcDbEntity* MirrorEnt(AcDbEntity* pEnt, const AcGePoint3d& pt1, const AcGePoint3d& pt2);
    AcArray<AcDbEntity*> MirrorEnt(const AcDbObjectIdArray& arrid, const AcGePoint3d& pt1, const AcGePoint3d& pt2);
    AcArray<AcDbEntity*> MirrorEnt(const AcArray<AcDbEntity*>& arrp, const AcGePoint3d& pt1, const AcGePoint3d& pt2);

    /// @brief 环形矩阵
    /// @param nCurNum 当前需要的数量
    /// @param nSum 总共的数量
    AcArray<AcDbEntity*> AnnularMatrix(const AcDbObjectIdArray& arrid, const AcGePoint3d& ptBase, int nCurNum, int nSum);
    AcArray<AcDbEntity*> AnnularMatrix(const AcArray<AcDbEntity*>& arrp, const AcGePoint3d& ptBase, int nCurNum, int nSum);

    /// @brief 设置实体被选中
    void SetEntSelected(const AcDbObjectIdArray& arridEnt, bool bSeled = true, bool bHighlight = false);

    /// @brief 调整绘图次序
    bool SetEntToBottom(const AcDbObjectId& id, AcDbDatabase* pDb = acdbCurDwg());

    /// @brief 拉伸实体
    /// @param arrid        输入需要被拉伸的实体id数组
    /// @param ptCorner1    输入被拉伸的角点1
    /// @param ptCorner2    输入被拉伸的角点2
    /// @param ptBase       输入基点
    /// @param ptTarget     输入目标点
    /// @return
    bool StretchEnt(const AcDbObjectIdArray& arrid, const AcGePoint3d& ptCorner1, const AcGePoint3d& ptCorner2, const AcGePoint3d& ptBase, const AcGePoint3d& ptTarget);

    /// @brief 得到经过点坐标上的实体id
    AcDbObjectIdArray GetEntIdByPt(const AcGePoint3d& pt, bool bAll = false);

    /// @brief 得到实体外包矩形
    AcDbExtents GetEntExtents(const AcDbObjectId& idEnt);
    AcDbExtents GetEntExtents(const AcDbObjectIdArray& arrid);
    AcDbExtents GetEntExtents(const AcArray<AcDbEntity*>& arrEnt);

    /// @brief 得到实体外包矩形的最大点和最小点
    bool GetExtentsPt(const AcDbObjectId& idEnt, AcGePoint3d& ptMax, AcGePoint3d& ptMin);
    bool GetExtentsPt(AcDbEntity* pEnt, AcGePoint3d& ptMax, AcGePoint3d& ptMin);
    bool GetExtentsPt(const AcDbObjectIdArray& arrid, AcGePoint3d& ptMax, AcGePoint3d& ptMin);

#pragma region polyline
    /// @brief 判断该多段线是否是闭合多段线
    bool IsClosedPline(AcDbPolyline* pPline);
#pragma endregion

   private:
    std::shared_ptr<KTArxConvert> m_pArxConvert;
};
}  // namespace KTArxTool
