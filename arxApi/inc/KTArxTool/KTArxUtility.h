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
    };
} // namespace KTARXTOOL
