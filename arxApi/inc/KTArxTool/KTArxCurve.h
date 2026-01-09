#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxCurve {
   public:
    /// @brief   得到向量方向在线段左右的正负值
    /// @details ptBase 沿 vecDir 偏移后，是否属于曲线的“正向”
    bool GetPosAngNegValue(AcDbCurve* pCurve, const AcGePoint3d& ptBase, const AcGeVector3d& vecDir);

    /// @brief 偏移线段
    AcArray<AcDbCurve*> OffsetCurve(AcDbCurve* pCurve, double dOffset);
    /// @details 通过 pt 自动计算偏移方向
    AcArray<AcDbCurve*> OffsetCurve(AcDbCurve* pCurve, double dOffset, const AcGePoint3d& pt, bool bOpposite = false);

    /// @brief 得到线段上距离该点最近的点
    AcGePoint3d GetClosedPtInCurve(AcDbCurve* pCurve, const AcGePoint3d& pt);
    bool GetClosedPtInCurve(const AcDbObjectId& idCurve, const AcGePoint3d& pt, AcGePoint3d& ptClosed);

    /// @brief 打断线段
    /// @details 输入的点必须要用getClosestPointTo函数重新获得
    AcArray<AcDbCurve*> SplitCurve(AcDbCurve* pCurve, const AcGePoint3dArray& arrSplitPt);
    AcArray<AcDbCurve*> SplitCurve(const AcDbObjectId& idCurve, const AcGePoint3dArray& arrSplitPt);
    AcArray<AcDbCurve*> SplitCurve(AcDbCurve* pCurve, const AcGePoint3d& ptSplit);

    /// @brief 通过距离延伸线段
    bool ExtendCurveByDist(AcDbPolyline* pPline, bool isStartPt, double dDist);
    /// @brief 延伸线段
    /// @details 扩展一条曲线(pExtend), 使其与另一条边界曲线(pBorder)相交
    bool ExtendCurve(AcDbCurve* pBorder, AcDbCurve* pExtend, const AcGePoint3d& ptExtend);

    /// @brief 修剪线段
    /// @details 修剪一条曲线(pTrim), 使其与另一条边界曲线(pBorder)相交, 修剪点 ptTrim, 修剪后的新曲线 pNewTrim
    bool TrimCurve(AcDbCurve* pBorder, AcDbCurve* pTrim, const AcGePoint3d& ptTrim, AcDbCurve*& pNewTrim);

    /// @brief 判断点是否在线段上
    bool IsPtOnCurve(const AcGePoint3d& pt, AcDbCurve* pCurve, double dTol = 0);

    /// @brief 得到圆弧凸度
    double GetArcBulge(AcDbArc* pArc);
    double GetArcBulge(double dAngleStart, double dAngleEnd);
    double GetArcBulge(const AcDbObjectId& idArc);
    double GetArcBulge(AcDbCurve* pCurve);
    double GetArcBulge(const AcGeCircArc2d& geArc);

    /// @brief 得到几何类对象
    AcGeLine2d GetGeLine2d(AcDbLine* pLine);
    AcGeLineSeg2d GetGeLineSeg2d(AcDbLine* pLine);
    AcGeLineSeg2d GetGeLineSeg2d(const AcDbObjectId& idLine);
    AcGeLineSeg2d GetGeLineSeg2d(AcDbPolyline* pPolyline, unsigned int unIndex);
    AcGeCircArc2d GetGeCircArc2d(AcDbArc* pArc);
    AcGeCircArc2d GetGeCircArc2d(AcDbCircle* pCircle);
    AcGeCircArc2d GetGeCircArc2d(AcDbPolyline* pPolyline, unsigned int unIndex);

    /// @brief 得到交点
    bool GetIntersectPoint(const AcGeLine2d& geLine1, const AcGeLine2d& geLine2, AcGePoint3d& ptIntersect);
    bool GetIntersectPoint(const AcGeLineSeg2d& geLine1, const AcGeLineSeg2d& geLine2, AcGePoint3d& ptIntersect);
    bool GetIntersectPoint(const AcGeLineSeg2d& geLine1, const AcGeLine2d& geLine2, AcGePoint3d& ptIntersect);
    bool GetIntersectPoint(const AcGeCircArc2d& geCircArc1, const AcGeCircArc2d& geCircArc2, int& nNum, AcGePoint3dArray& arrptIntersect);
    bool GetIntersectPoint(const AcGeLine2d& geLine, const AcGeCircArc2d& geCircArc, int& nNum, AcGePoint3dArray& arrptIntersect);
    bool GetIntersectPoint(const AcGeLineSeg2d& geLine, const AcGeCircArc2d& geCircArc, int& nNum, AcGePoint3dArray& arrptIntersect);

    /// @brief 得到向量在另一向量上的投影向量
    /// @param ptBase 	    输入基点
    /// @param vctOffset    输入被投影的向量
    /// @param vctDirection 输入方向向量
    /// @return
    AcGeVector3d GetProjectVector(const AcGePoint3d& ptBase, const AcGeVector3d& vctOffset, const AcGeVector3d& vctDirection);

    /// @brief 得到线段某点的切线向量
    AcGeVector3d GetTangentLineVec(const AcGePoint3d& ptOnCurve, const AcDbObjectId& idCurve);
    bool GetTangentLineVecArrForPolyline(AcDbPolyline* pPline, const AcGePoint3d& pt, AcGeVector3dArray& arrvec);
    AcGeVector3dArray GetTangentLineVecArr(const AcGePoint3d& pt, AcDbCurve* pCurve);
    AcGeVector3dArray GetTangentLineVecArr(const AcGePoint3d& pt, const AcDbObjectId& idCurve);

    /// @brief 得到两直线的角平分线向量
    /// @param pLine1 输入直线1的id
    /// @param pLine2 输入直线2的id
    /// @return 输出角平分线向量
    AcGeVector2d GetMidVectorOfLines(AcDbLine* pLine1, AcDbLine* pLine2);

    /// @brief 通过向量和点得到和X轴的角度(0~2π)
    double GetAngle(const AcGeVector3d& vec);

    /// @brief 得到线段长度
    double GetCurveLength(AcDbCurve* pCurve);

    /// @brief 得到两直线的夹角
    double GetAngleOfLines(AcDbLine* pLine1, AcDbLine* pLine2);

   private:
    std::shared_ptr<KTArxEntity> m_pArxEntity;
    std::shared_ptr<KTArxConvert> m_pArxConvert;
};
}  // namespace KTArxTool
