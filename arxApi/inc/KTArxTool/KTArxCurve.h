#pragma once

namespace KTArxTool {
class KTArxEntity;
class KTArxConvert;
class KT_EXPORTARX_API KTArxCurve {
   public:
    /// @brief   得到向量方向在线段左右的正负值
    /// @details ptBase 沿 vecDir 偏移后，是否属于曲线的“正向”
    static bool getPosAngNegValue(AcDbCurve* curve, const AcGePoint3d& basePoint, const AcGeVector3d& directionVector);

    /// @brief 偏移线段
    static AcArray<AcDbCurve*> offsetCurve(AcDbCurve* curve, double offset);
    /// @details 通过 pt 自动计算偏移方向
    static AcArray<AcDbCurve*> offsetCurve(AcDbCurve* curve, double offset, const AcGePoint3d& point, bool opposite = false);

    /// @brief 得到线段上距离该点最近的点
    static AcGePoint3d getClosedPtInCurve(AcDbCurve* curve, const AcGePoint3d& point);
    static bool getClosedPtInCurve(const AcDbObjectId& curveId, const AcGePoint3d& point, AcGePoint3d& closestPoint);

    /// @brief 打断线段
    /// @details 输入的点必须要用getClosestPointTo函数重新获得
    static AcArray<AcDbCurve*> splitCurve(AcDbCurve* pCurve, const AcGePoint3dArray& arrSplitPt);
    static AcArray<AcDbCurve*> splitCurve(const AcDbObjectId& idCurve, const AcGePoint3dArray& arrSplitPt);
    static AcArray<AcDbCurve*> splitCurve(AcDbCurve* pCurve, const AcGePoint3d& ptSplit);

    /// @brief 通过距离延伸线段
    static bool extendCurveByDist(AcDbPolyline* polyline, bool isStartPoint, double distance);
    /// @brief 延伸线段
    /// @details 扩展一条曲线(pExtend), 使其与另一条边界曲线(pBorder)相交
    static bool extendCurve(AcDbCurve* border, AcDbCurve* curveToExtend, const AcGePoint3d& extendPoint);

    /// @brief 修剪线段
    /// @details 修剪一条曲线(pTrim), 使其与另一条边界曲线(pBorder)相交, 修剪点 ptTrim, 修剪后的新曲线 pNewTrim
    static bool trimCurve(AcDbCurve* pBorder, AcDbCurve* pTrim, const AcGePoint3d& ptTrim, AcDbCurve*& pNewTrim);

    /// @brief 判断点是否在线段上
    static bool isPtOnCurve(const AcGePoint3d& point, AcDbCurve* curve, double tolerance = 0);

    /// @brief 得到圆弧凸度
    static double getArcBulge(AcDbArc* arc);
    static double getArcBulge(double startAngle, double endAngle);
    static double getArcBulge(const AcDbObjectId& arcId);
    static double getArcBulge(AcDbCurve* curve);
    static double getArcBulge(const AcGeCircArc2d& arc);

    /// @brief 得到几何类对象
    static AcGeLine2d getGeLine2d(AcDbLine* line);
    static AcGeLineSeg2d getGeLineSeg2d(AcDbLine* line);
    static AcGeLineSeg2d getGeLineSeg2d(const AcDbObjectId& lineId);
    static AcGeLineSeg2d getGeLineSeg2d(AcDbPolyline* polyline, unsigned int index);
    static AcGeCircArc2d getGeCircArc2d(AcDbArc* arc);
    static AcGeCircArc2d getGeCircArc2d(AcDbCircle* circle);
    static AcGeCircArc2d getGeCircArc2d(AcDbPolyline* polyline, unsigned int index);

    /// @brief 得到交点
    static bool getIntersectPoint(const AcGeLine2d& line1, const AcGeLine2d& line2, AcGePoint3d& intersectionPoint);
    static bool getIntersectPoint(const AcGeLineSeg2d& lineSeg1, const AcGeLineSeg2d& lineSeg2, AcGePoint3d& intersectionPoint);
    static bool getIntersectPoint(const AcGeLineSeg2d& lineSeg, const AcGeLine2d& line, AcGePoint3d& intersectionPoint);
    static bool getIntersectPoint(const AcGeCircArc2d& arc1, const AcGeCircArc2d& arc2, int& intersectionCount, AcGePoint3dArray& intersectionPoints);
    static bool getIntersectPoint(const AcGeLine2d& line, const AcGeCircArc2d& arc, int& intersectionCount, AcGePoint3dArray& intersectionPoints);
    static bool getIntersectPoint(const AcGeLineSeg2d& lineSeg, const AcGeCircArc2d& arc, int& intersectionCount, AcGePoint3dArray& intersectionPoints);

    /// @brief 得到向量在另一向量上的投影向量
    /// @param ptBase 	    输入基点
    /// @param vctOffset    输入被投影的向量
    /// @param vctDirection 输入方向向量
    /// @return
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    static AcGeVector3d getProjectVector(const AcGePoint3d& basePoint, const AcGeVector3d& offsetVector, const AcGeVector3d& directionVector);

    /// @brief 得到线段某点的切线向量
    static AcGeVector3d getTangentLineVec(const AcGePoint3d& ptOnCurve, const AcDbObjectId& idCurve);
    static bool getTangentLineVecArrForPolyline(AcDbPolyline* polyline, const AcGePoint3d& point, AcGeVector3dArray& tangentVectors);
    static AcGeVector3dArray getTangentLineVecArr(const AcGePoint3d& point, AcDbCurve* curve);
    static AcGeVector3dArray getTangentLineVecArr(const AcGePoint3d& point, const AcDbObjectId& curveId);

    /// @brief 得到两直线的角平分线向量
    /// @param pLine1 输入直线1的id
    /// @param pLine2 输入直线2的id
    /// @return 输出角平分线向量
    static AcGeVector2d getMidVectorOfLines(AcDbLine* pLine1, AcDbLine* pLine2);

    /// @brief 通过向量和点得到和X轴的角度(0~2π)
    static double getAngle(const AcGeVector3d& vector);

    /// @brief 得到线段长度
    static double getCurveLength(AcDbCurve* curve);

    /// @brief 得到两直线的夹角
    static double getAngleOfLines(AcDbLine* pLine1, AcDbLine* pLine2);

   private:
    std::shared_ptr<KTArxEntity> m_pArxEntity;
    std::shared_ptr<KTArxConvert> m_pArxConvert;
};
}  // namespace KTArxTool
