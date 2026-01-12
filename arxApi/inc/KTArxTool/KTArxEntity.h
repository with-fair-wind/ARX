#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxEntity {
   public:
    /// @brief 按比例缩放实体
    static bool modifyEntByScale(AcDbEntity* entity, double scale, const AcGePoint3d& basePoint);
    static bool modifyEntByScale(const AcDbObjectId& entityId, double scale, const AcGePoint3d& basePoint);
    static bool modifyEntByScale(const AcArray<AcDbEntity*>& entities, double scale, const AcGePoint3d& basePoint);

    /// @brief 分解实体
    static AcArray<AcDbEntity*> explodeEnt(AcDbEntity* entity);
    static AcArray<AcDbEntity*> explodeEnt(const AcDbObjectId& entityId);

    /// @brief 移动实体
    static bool moveEnt(AcDbEntity* entity, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint);
    static bool moveEnt(const AcDbObjectIdArray& entityIds, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint);
    static bool moveEnt(const AcDbObjectId& entityId, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint);
    static bool moveEnt(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& basePoint, const AcGePoint3d& destPoint);

    /// @brief 旋转实体(弧度)
    static bool rotateEnt(AcDbEntity* entity, const AcGePoint3d& basePoint, double rotation);
    static bool rotateEnt(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& basePoint, double rotation);

    /// @brief 删除实体
    static bool deleteEnt(const AcDbObjectId& entityId);
    static void deleteEnt(AcArray<AcDbEntity*>& entities);

    /// @brief 拷贝实体
    static AcArray<AcDbEntity*> copyEnt(const AcDbObjectIdArray& entityIds);
    static AcArray<AcDbEntity*> copyEnt(const AcArray<AcDbEntity*>& entities);
    static AcDbEntity* copyEnt(const AcDbObjectId& entityId);

    /// @brief 镜像实体
    static AcDbEntity* mirrorEnt(AcDbEntity* entity, const AcGePoint3d& point1, const AcGePoint3d& point2);
    static AcArray<AcDbEntity*> mirrorEnt(const AcDbObjectIdArray& entityIds, const AcGePoint3d& point1, const AcGePoint3d& point2);
    static AcArray<AcDbEntity*> mirrorEnt(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& point1, const AcGePoint3d& point2);

    /// @brief 环形矩阵
    /// @param nCurNum 当前需要的数量
    /// @param nSum 总共的数量
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    static AcArray<AcDbEntity*> annularMatrix(const AcDbObjectIdArray& entityIds, const AcGePoint3d& basePoint, int currentCount, int totalCount);
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    static AcArray<AcDbEntity*> annularMatrix(const AcArray<AcDbEntity*>& entities, const AcGePoint3d& basePoint, int currentCount, int totalCount);

    /// @brief 设置实体被选中
    static void setEntSelected(const AcDbObjectIdArray& entityIds, bool selected = true, bool highlight = false);

    /// @brief 调整绘图次序
    static bool setEntToBottom(const AcDbObjectId& entityId, AcDbDatabase* database = acdbCurDwg());

    /// @brief 拉伸实体
    /// @param arrid        输入需要被拉伸的实体id数组
    /// @param ptCorner1    输入被拉伸的角点1
    /// @param ptCorner2    输入被拉伸的角点2
    /// @param ptBase       输入基点
    /// @param ptTarget     输入目标点
    /// @return
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    static bool stretchEnt(const AcDbObjectIdArray& entityIds, const AcGePoint3d& cornerPoint1, const AcGePoint3d& cornerPoint2, const AcGePoint3d& basePoint, const AcGePoint3d& targetPoint);

    /// @brief 得到经过点坐标上的实体id
    static AcDbObjectIdArray getEntIdByPt(const AcGePoint3d& point, bool all = false);

    /// @brief 得到实体外包矩形
    static AcDbExtents getEntExtents(const AcDbObjectId& entityId);
    static AcDbExtents getEntExtents(const AcDbObjectIdArray& entityIds);
    static AcDbExtents getEntExtents(const AcArray<AcDbEntity*>& entities);

    /// @brief 得到实体外包矩形的最大点和最小点
    static bool getExtentsPt(const AcDbObjectId& entityId, AcGePoint3d& maxPoint, AcGePoint3d& minPoint);
    static bool getExtentsPt(AcDbEntity* entity, AcGePoint3d& maxPoint, AcGePoint3d& minPoint);
    static bool getExtentsPt(const AcDbObjectIdArray& entityIds, AcGePoint3d& maxPoint, AcGePoint3d& minPoint);

#pragma region polyline
    /// @brief 判断该多段线是否是闭合多段线
    static bool isClosedPline(AcDbPolyline* polyline);
#pragma endregion
};
}  // namespace KTArxTool
