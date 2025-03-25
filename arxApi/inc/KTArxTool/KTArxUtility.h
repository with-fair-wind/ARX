#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUtility
    {
    public:
        // Summary:   得到当前项目zrx文件路径
        CString GetCurZrxFilePath();

        // Summary:   得到当前项目zrx文件所在目录路径
        // Explain:   返回的目录路径含带'\\'
        CString GetCurZrxFolderPath();

        // Summary:   加载cad菜单文件
        bool LoadCadMenu(const CString &strMenuFileName, const CString &strMenuGroupName);

        // Summary:   卸载cad菜单
        bool UnloadCadMenu(const CString &strMenuGroupName);

        // Summary:   加载zrx文件
        bool LoadZrxFile(LPCTSTR szFilePath);

        // Summary:   添加搜索路径
        bool AddSearchPath(LPCTSTR szSearchDir);

        // Summary:   将实体添加到模型空间
        AcDbObjectId PostToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());

        // Summary:   按比例缩放实体
        bool ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale, const AcGePoint3d &ptBase);

        // Summary:   分解实体
        AcArray<AcDbEntity *> ExplodeEnt(AcDbEntity *pEnt);
        AcArray<AcDbEntity *> ExplodeEnt(const AcDbObjectId &idEnt);

        // Summary:   移动实体
        bool MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);

        // Summary:   旋转实体(弧度)
        bool RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation);
        bool RotateEnt(const AcArray<AcDbEntity *> &arrpEnt, const AcGePoint3d &ptBase, double rotation);

        // Summary:   得到向量方向在线段左右的正负值
        // Explain:	  ptBase 沿 vecDir 偏移后，是否属于曲线的“正向”
        bool GetPosAngNegValue(AcDbCurve *pCurve, const AcGePoint3d &ptBase, const AcGeVector3d &vecDir);

        // Summary:   偏移线段
        AcArray<AcDbCurve *> OffsetCurve(AcDbCurve *pCurve, double dOffset);
        // Explain:	  通过 pt 自动计算偏移方向
        AcArray<AcDbCurve *> OffsetCurve(AcDbCurve *pCurve, double dOffset, const AcGePoint3d &pt, bool bOpposite = false);

        // Summary:   得到线段上距离该点最近的点
        AcGePoint3d GetClosedPtInCurve(AcDbCurve *pCurve, const AcGePoint3d &pt);
        bool GetClosedPtInCurve(const AcDbObjectId &idCurve, const AcGePoint3d &pt, AcGePoint3d &ptClosed);

        // Summary:   打断线段
        // Explain:	  输入的点必须要用getClosestPointTo函数重新获得
        AcArray<AcDbCurve *> SplitCurve(AcDbCurve *pCurve, const AcGePoint3dArray &arrSplitPt);
        AcArray<AcDbCurve *> SplitCurve(const AcDbObjectId &idCurve, const AcGePoint3dArray &arrSplitPt);
        AcArray<AcDbCurve *> SplitCurve(AcDbCurve *pCurve, const AcGePoint3d &ptSplit);

        // Summary:   通过距离延伸线段
        bool ExtendCurveByDist(AcDbPolyline *pPline, bool isStartPt, double dDist);
        // Summary:   延伸线段
        // Explain:   扩展一条曲线(pExtend), 使其与另一条边界曲线(pBorder)相交
        bool ExtendCurve(AcDbCurve *pBorder, AcDbCurve *pExtend, const AcGePoint3d &ptExtend);

        // Summary:   修剪线段
        // Explain:   修剪一条曲线(pTrim), 使其与另一条边界曲线(pBorder)相交, 修剪点 ptTrim, 修剪后的新曲线 pNewTrim
        bool TrimCurve(AcDbCurve *pBorder, AcDbCurve *pTrim, const AcGePoint3d &ptTrim, AcDbCurve *&pNewTrim);

        // Summary:   判断点是否在线段上
        bool IsPtOnCurve(const AcGePoint3d &pt, AcDbCurve *pCurve, double dTol = 0);

        // Summary:   得到圆弧凸度
        double GetArcBulge(AcDbArc *pArc);
        double GetArcBulge(double dAngleStart, double dAngleEnd);
        double GetArcBulge(const AcDbObjectId &idArc);
        double GetArcBulge(AcDbCurve *pCurve);
        double GetArcBulge(const AcGeCircArc2d &geArc);

        // Summary:   得到几何类对象
        AcGeLine2d GetGeLine2d(AcDbLine *pLine);
        AcGeLineSeg2d GetGeLineSeg2d(AcDbLine *pLine);
        AcGeLineSeg2d GetGeLineSeg2d(const AcDbObjectId &idLine);
        AcGeLineSeg2d GetGeLineSeg2d(AcDbPolyline *pPolyline, unsigned int unIndex);
        AcGeCircArc2d GetGeCircArc2d(AcDbArc *pArc);
        AcGeCircArc2d GetGeCircArc2d(AcDbCircle *pCircle);
        AcGeCircArc2d GetGeCircArc2d(AcDbPolyline *pPolyline, unsigned int unIndex);

        // Summary:   得到交点
        bool GetIntersectPoint(const AcGeLine2d &geLine1, const AcGeLine2d &geLine2,
                               AcGePoint3d &ptIntersect);
        bool GetIntersectPoint(const AcGeLineSeg2d &geLine1, const AcGeLineSeg2d &geLine2,
                               AcGePoint3d &ptIntersect);
        bool GetIntersectPoint(const AcGeLineSeg2d &geLine1, const AcGeLine2d &geLine2,
                               AcGePoint3d &ptIntersect);
        bool GetIntersectPoint(const AcGeCircArc2d &geCircArc1, const AcGeCircArc2d &geCircArc2,
                               int &nNum, AcGePoint3dArray &arrptIntersect);
        bool GetIntersectPoint(const AcGeLine2d &geLine, const AcGeCircArc2d &geCircArc,
                               int &nNum, AcGePoint3dArray &arrptIntersect);
        bool GetIntersectPoint(const AcGeLineSeg2d &geLine, const AcGeCircArc2d &geCircArc,
                               int &nNum, AcGePoint3dArray &arrptIntersect);

        /// @brief 得到向量在另一向量上的投影向量
        /// @param ptBase 	    输入基点
        /// @param vctOffset    输入被投影的向量
        /// @param vctDirection 输入方向向量
        /// @return
        AcGeVector3d GetProjectVector(const AcGePoint3d &ptBase, const AcGeVector3d &vctOffset,
                                      const AcGeVector3d &vctDirection);
    };
} // namespace KTARXTOOL
