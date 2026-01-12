#include <KTArxTool/KTArxConvert.h>
#include <KTArxTool/KTArxCurve.h>
#include <KTArxTool/KTArxEntity.h>

#include <cassert>
#include <cmath>

namespace KTArxTool {
bool KTArxCurve::GetPosAngNegValue(AcDbCurve* curve, const AcGePoint3d& basePoint, const AcGeVector3d& directionVector) {
    assert(curve != nullptr);

    const AcGePoint3d ptNew = basePoint + directionVector;
    bool isPositive = true;

    // 算出实体平面法向量
    AcGePlane plane;
    AcDb::Planarity type = AcDb::kNonPlanar;
    curve->getPlane(plane, type);
    AcGeVector3d vecNormalNew = plane.normal();
    // 根据偏移点（鼠标点），找出矩形上距离该点最近的点ptClosed
    AcGePoint3d ptClosed;
    curve->getClosestPointTo(ptNew, ptClosed);
    // 算出矩形上经过ptClosed的切线，即经过该点的那条边vecDerive
    AcGeVector3d vecDerive;
    curve->getFirstDeriv(ptClosed, vecDerive);
    // 使用ptClosed得到一个偏移向量vecOffsetdir
    AcGeVector3d vecOffsetdir = ptNew - ptClosed;
    // vecOffset和vecDerive做叉乘，算出法向量vecOffsetNormal，看vecOffsetNormal和
    // vecNormal是否同向，如果同向偏移距离为dOffset，如果不同向偏移距离为-dOffset
    AcGeVector3d vecOffsetNormal = vecOffsetdir.crossProduct(vecDerive);
    bool bCodirectional = vecOffsetNormal.isCodirectionalTo(vecNormalNew);
    if (!bCodirectional) {
        isPositive = !isPositive;
    }
    // 如果是直线，取反
    if (curve->isKindOf(AcDbLine::desc())) {
        isPositive = !isPositive;
    }

    return isPositive;
}

AcArray<AcDbCurve*> KTArxCurve::OffsetCurve(AcDbCurve* curve, double offset) {
    AcArray<AcDbCurve*> arrOffsetCurve;
    AcDbVoidPtrArray osArr;
    if (curve->getOffsetCurves(offset, osArr) == Acad::eOk) {
        for (int i = 0; i < osArr.length(); ++i) {
            arrOffsetCurve.append(static_cast<AcDbCurve*>(osArr.at(i)));
        }
    }
    return arrOffsetCurve;
}

AcArray<AcDbCurve*> KTArxCurve::OffsetCurve(AcDbCurve* curve, double offset, const AcGePoint3d& point, bool opposite) {
    offset = std::fabs(offset);  // 绝对值
    // 算出实体平面法向量
    AcGePlane plane;
    AcDb::Planarity type = AcDb::kNonPlanar;
    curve->getPlane(plane, type);
    AcGeVector3d vecNormal = plane.normal();
    // 根据偏移点（鼠标点），找出矩形上距离该点最近的点ptClosed
    AcGePoint3d ptClosed = GetClosedPtInCurve(curve, point);
    // 算出矩形上经过ptClosed的切线，即经过该点的那条边vecDerive
    AcGeVector3d vecDerive;
    curve->getFirstDeriv(ptClosed, vecDerive);
    // 使用ptClosed得到一个偏移向量vecOffsetdir
    AcGeVector3d vecOffsetdir = point - ptClosed;
    // vecOffset和vecDerive做叉乘，算出法向量vecOffsetNormal，看vecOffsetNormal和
    // vecNormal是否同向，如果同向偏移距离为dOffset，如果不同向偏移距离为-dOffset
    AcGeVector3d vecOffsetNormal = vecOffsetdir.crossProduct(vecDerive);
    bool bCodirectional = vecOffsetNormal.isCodirectionalTo(vecNormal);
    if (!bCodirectional) {
        offset = -offset;
    }
    // 如果是直线，取反
    if (curve->isKindOf(AcDbLine::desc())) {
        offset = -offset;
    }
    if (opposite) {
        offset = -offset;
    }
    // 偏移实体
    return OffsetCurve(curve, offset);
}

AcGePoint3d KTArxCurve::GetClosedPtInCurve(AcDbCurve* curve, const AcGePoint3d& point) {
    double dist = 0.0;
    if (Acad::eInvalidInput == curve->getDistAtPoint(point, dist)) {
        AcGePoint3d ptOnCurve;
        curve->getClosestPointTo(point, ptOnCurve);
        return ptOnCurve;
    }
    return point;
}

bool KTArxCurve::GetClosedPtInCurve(const AcDbObjectId& curveId, const AcGePoint3d& point, AcGePoint3d& closestPoint) {
    AcDbObjectPointer<AcDbCurve> curve(curveId, AcDb::kForRead);
    if (Acad::eOk != curve.openStatus()) {
        return false;
    }
    closestPoint = GetClosedPtInCurve(curve, point);
    return true;
}

AcArray<AcDbCurve*> KTArxCurve::SplitCurve(AcDbCurve* pCurve, const AcGePoint3dArray& arrSplitPt) {
    AcArray<AcDbCurve*> arrSplitCurve;
    AcDbVoidPtrArray spArr;
    AcGePoint3dArray arrptClosed;
    for (int i = 0; i < arrSplitPt.length(); i++) arrptClosed.append(GetClosedPtInCurve(pCurve, arrSplitPt.at(i)));

    if (pCurve->getSplitCurves(arrptClosed, spArr) == Acad::eOk) {
        for (int i = 0; i < spArr.length(); i++) arrSplitCurve.append(static_cast<AcDbCurve*>(spArr.at(i)));
    }
    return arrSplitCurve;
}

AcArray<AcDbCurve*> KTArxCurve::SplitCurve(const AcDbObjectId& idCurve, const AcGePoint3dArray& arrSplitPt) {
    AcArray<AcDbCurve*> arrpSplit;
    AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
    if (Acad::eOk != pCurve.openStatus()) return arrpSplit;
    return SplitCurve(pCurve, arrSplitPt);
}

AcArray<AcDbCurve*> KTArxCurve::SplitCurve(AcDbCurve* pCurve, const AcGePoint3d& ptSplit) {
    AcGePoint3dArray arrSplitPt;
    arrSplitPt.append(ptSplit);
    return SplitCurve(pCurve, arrSplitPt);
}

bool KTArxCurve::ExtendCurveByDist(AcDbPolyline* polyline, bool isStartPoint, double distance) {
    if (polyline->numVerts() < 2) {
        return false;
    }

    if (isStartPoint) {
        AcGePoint3d startPoint;
        AcGePoint3d nextPoint;
        polyline->getStartPoint(startPoint);
        polyline->getPointAt(1, nextPoint);

        double startDist = 0.0;
        double nextDist = 0.0;
        polyline->getDistAtPoint(startPoint, startDist);
        polyline->getDistAtPoint(nextPoint, nextDist);

        const double segDist = nextDist - startDist;
        if (segDist == 0.0) {
            return false;
        }

        const double startParam = std::fabs(distance) / segDist;
        return polyline->extend(-startParam) == Acad::eOk;
    }

    AcGePoint3d endPoint;
    AcGePoint3d prevPoint;
    polyline->getEndPoint(endPoint);
    polyline->getPointAt(polyline->numVerts() - 2, prevPoint);

    double prevDist = 0.0;
    double endDist = 0.0;
    polyline->getDistAtPoint(prevPoint, prevDist);
    polyline->getDistAtPoint(endPoint, endDist);

    const double segDist = endDist - prevDist;
    if (segDist == 0.0) {
        return false;
    }

    const double endParam = std::fabs(distance) / segDist;
    return polyline->extend(endParam) == Acad::eOk;
}

bool KTArxCurve::ExtendCurve(AcDbCurve* border, AcDbCurve* curveToExtend, const AcGePoint3d& extendPoint) {
    AcGePoint3dArray arrptInter;
    curveToExtend->intersectWith(border, AcDb::kExtendThis, arrptInter);
    if (arrptInter.isEmpty()) {
        return false;
    }
    AcGePoint3d ptStart, ptEnd;
    curveToExtend->getStartPoint(ptStart);
    curveToExtend->getEndPoint(ptEnd);
    double dDistStart = extendPoint.distanceTo(ptStart);
    double dDistEnd = extendPoint.distanceTo(ptEnd);
    return curveToExtend->extend(dDistStart < dDistEnd, arrptInter[0]) == Acad::eOk;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool KTArxCurve::TrimCurve(AcDbCurve* pBorder, AcDbCurve* pTrim, const AcGePoint3d& ptTrim, AcDbCurve*& pNewTrim) {
    AcGePoint3dArray arrptInter;
    pTrim->intersectWith(pBorder, AcDb::kOnBothOperands, arrptInter);
    if (arrptInter.isEmpty()) return false;

    AcArray<AcDbCurve*> arrSplit = SplitCurve(pTrim, arrptInter[0]);
    if (arrSplit.length() != 2) {
        for (int i = 0; i < arrSplit.length(); i++) DEL(arrSplit[i]);
        return false;
    }

    if (IsPtOnCurve(ptTrim, arrSplit[0])) {
        pNewTrim = arrSplit[1];
        DEL(arrSplit[0]);
        return true;
    }

    if (IsPtOnCurve(ptTrim, arrSplit[1])) {
        pNewTrim = arrSplit[0];
        DEL(arrSplit[1]);
        return true;
    }

    for (int i = 0; i < arrSplit.length(); i++) DEL(arrSplit[i]);
    return false;
}

bool KTArxCurve::IsPtOnCurve(const AcGePoint3d& point, AcDbCurve* curve, double tolerance) {
    double dist = 0.0;
    if (Acad::eInvalidInput != curve->getDistAtPoint(point, dist)) {
        return true;
    }

    if (tolerance <= 0.0) {
        return false;
    }

    const AcGePoint3d ptOnCurve = GetClosedPtInCurve(curve, point);
    AcGeTol tol;
    tol.setEqualPoint(tolerance);
    return point.isEqualTo(ptOnCurve, tol);
}

double KTArxCurve::GetArcBulge(AcDbArc* arc) { return GetArcBulge(arc->startAngle(), arc->endAngle()); }

double KTArxCurve::GetArcBulge(double startAngle, double endAngle) {
    constexpr double kBulgeDivisor = 4.0;
    constexpr double kTwoPi = 2.0 * M_PI;

    double deltaAngle = endAngle - startAngle;
    if (deltaAngle < 0.0) {  // 如果终点角度小于起点角度;
        deltaAngle += kTwoPi;
    }
    return std::tan(deltaAngle / kBulgeDivisor);
}

double KTArxCurve::GetArcBulge(const AcGeCircArc2d& arc) { return GetArcBulge(arc.startAng(), arc.endAng()); }

double KTArxCurve::GetArcBulge(AcDbCurve* curve) {
    double dBulge = 0.0;
    if (curve->isKindOf(AcDbArc::desc())) {
        AcDbArc* arc = AcDbArc::cast(curve);
        dBulge = GetArcBulge(arc->startAngle(), arc->endAngle());
    }
    return dBulge;
}

double KTArxCurve::GetArcBulge(const AcDbObjectId& arcId) {
    AcDbObjectPointer<AcDbArc> arc(arcId, AcDb::kForRead);
    if (Acad::eOk != arc.openStatus()) {
        return 0.0;
    }
    return GetArcBulge(arc->startAngle(), arc->endAngle());
}

AcGeLine2d KTArxCurve::GetGeLine2d(AcDbLine* line) {
    AcGePoint3d ptStart = line->startPoint();
    AcGePoint3d ptEnd = line->endPoint();
    AcGeLine2d geLine;
    geLine.set(ptStart.convert2d(AcGePlane::kXYPlane), ptEnd.convert2d(AcGePlane::kXYPlane));
    return geLine;
}

AcGeLineSeg2d KTArxCurve::GetGeLineSeg2d(AcDbLine* line) {
    AcGePoint3d ptStart = line->startPoint();
    AcGePoint3d ptEnd = line->endPoint();
    AcGeLineSeg2d geLine;
    geLine.set(ptStart.convert2d(AcGePlane::kXYPlane), ptEnd.convert2d(AcGePlane::kXYPlane));
    return geLine;
}

AcGeLineSeg2d KTArxCurve::GetGeLineSeg2d(AcDbPolyline* polyline, unsigned int index) {
    AcGeLineSeg2d geLine;
    const Acad::ErrorStatus errorStatus = polyline->getLineSegAt(index, geLine);
    if (errorStatus != Acad::eOk) {
        return AcGeLineSeg2d{};
    }
    return geLine;
}

AcGeLineSeg2d KTArxCurve::GetGeLineSeg2d(const AcDbObjectId& lineId) {
    AcGeLineSeg2d geLine;
    AcDbObjectPointer<AcDbLine> line(lineId, AcDb::kForRead);
    if (Acad::eOk != line.openStatus()) {
        return geLine;
    }
    AcGePoint3d ptStart = line->startPoint();
    AcGePoint3d ptEnd = line->endPoint();
    geLine.set(ptStart.convert2d(AcGePlane::kXYPlane), ptEnd.convert2d(AcGePlane::kXYPlane));
    return geLine;
}

AcGeCircArc2d KTArxCurve::GetGeCircArc2d(AcDbPolyline* polyline, unsigned int index) {
    AcGeCircArc2d geCircArc;
    const Acad::ErrorStatus errorStatus = polyline->getArcSegAt(index, geCircArc);
    if (errorStatus != Acad::eOk) {
        return AcGeCircArc2d{};
    }
    return geCircArc;
}

AcGeCircArc2d KTArxCurve::GetGeCircArc2d(AcDbArc* arc) {
    AcGePoint3d centerPt = arc->center();
    double dRadius = arc->radius();
    double dStartAngle = arc->startAngle();
    double dEndAngle = arc->endAngle();
    AcGeCircArc2d geCircArc;
    geCircArc.set(centerPt.convert2d(AcGePlane::kXYPlane), dRadius, dStartAngle, dEndAngle);
    return geCircArc;
}

AcGeCircArc2d KTArxCurve::GetGeCircArc2d(AcDbCircle* circle) {
    AcGePoint3d centerPt = circle->center();
    double dRadius = circle->radius();
    AcGeCircArc2d geCircArc;
    geCircArc.set(centerPt.convert2d(AcGePlane::kXYPlane), dRadius);
    return geCircArc;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLine2d& line1, const AcGeLine2d& line2, AcGePoint3d& intersectionPoint) {
    AcGePoint2d pt2d;
    const bool isIntersected = line1.intersectWith(line2, pt2d);
    if (!isIntersected) {
        return false;
    }
    intersectionPoint = AcGePoint3d(pt2d.x, pt2d.y, 0);
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLineSeg2d& lineSeg1, const AcGeLineSeg2d& lineSeg2, AcGePoint3d& intersectionPoint) {
    AcGePoint2d pt2d;
    const bool isIntersected = lineSeg1.intersectWith(lineSeg2, pt2d);
    if (!isIntersected) {
        return false;
    }
    intersectionPoint = AcGePoint3d(pt2d.x, pt2d.y, 0);
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLineSeg2d& lineSeg, const AcGeLine2d& line, AcGePoint3d& intersectionPoint) {
    AcGePoint2d pt2d;
    const bool isIntersected = lineSeg.intersectWith(line, pt2d);
    if (!isIntersected) {
        return false;
    }
    intersectionPoint = AcGePoint3d(pt2d.x, pt2d.y, 0);
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeCircArc2d& arc1, const AcGeCircArc2d& arc2, int& intersectionCount, AcGePoint3dArray& intersectionPoints) {
    intersectionCount = 0;
    AcGePoint2d pt1, pt2;
    const bool isIntersected = arc1.intersectWith(arc2, intersectionCount, pt1, pt2);
    if (!isIntersected) {
        return false;
    }
    if (intersectionCount == 1) {
        intersectionPoints.append(AcGePoint3d(pt1.x, pt1.y, 0));
    } else if (intersectionCount == 2) {
        intersectionPoints.append(AcGePoint3d(pt1.x, pt1.y, 0));
        intersectionPoints.append(AcGePoint3d(pt2.x, pt2.y, 0));
    }
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLine2d& line, const AcGeCircArc2d& arc, int& intersectionCount, AcGePoint3dArray& intersectionPoints) {
    intersectionCount = 0;
    AcGePoint2d pt1, pt2;
    if (!arc.intersectWith(line, intersectionCount, pt1, pt2)) {
        return false;
    }
    if (intersectionCount == 1) {
        intersectionPoints.append(AcGePoint3d(pt1.x, pt1.y, 0));
    } else if (intersectionCount == 2) {
        intersectionPoints.append(AcGePoint3d(pt1.x, pt1.y, 0));
        intersectionPoints.append(AcGePoint3d(pt2.x, pt2.y, 0));
    }
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLineSeg2d& lineSeg, const AcGeCircArc2d& arc, int& intersectionCount, AcGePoint3dArray& intersectionPoints) {
    intersectionCount = 0;
    AcGePoint2d pt1, pt2;
    if (!arc.intersectWith(lineSeg, intersectionCount, pt1, pt2)) {
        return false;
    }
    if (intersectionCount == 1) {
        intersectionPoints.append(AcGePoint3d(pt1.x, pt1.y, 0));
    } else if (intersectionCount == 2) {
        intersectionPoints.append(AcGePoint3d(pt1.x, pt1.y, 0));
        intersectionPoints.append(AcGePoint3d(pt2.x, pt2.y, 0));
    }
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
AcGeVector3d KTArxCurve::GetProjectVector(const AcGePoint3d& basePoint, const AcGeVector3d& offsetVector, const AcGeVector3d& directionVector) {
    const AcGePoint3d offsetPoint = basePoint + offsetVector;
    const AcGeLine3d line(basePoint, directionVector);
    const AcGePoint3d projectedPoint = line.closestPointTo(offsetPoint);
    return projectedPoint - basePoint;
}

AcGeVector3d KTArxCurve::GetTangentLineVec(const AcGePoint3d& ptOnCurve, const AcDbObjectId& idCurve) {
    AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
    if (Acad::eOk != pCurve.openStatus()) {
        return AcGeVector3d::kIdentity;
    }
    const AcGePoint3d closestPoint = GetClosedPtInCurve(pCurve, ptOnCurve);
    AcGeVector3d vecTangLine;
    pCurve->getFirstDeriv(closestPoint, vecTangLine);
    return vecTangLine;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
bool KTArxCurve::GetTangentLineVecArrForPolyline(AcDbPolyline* polyline, const AcGePoint3d& point, AcGeVector3dArray& tangentVectors) {
    if (polyline == nullptr) {
        return false;
    }

    const AcGePoint3d pointOnCurve = GetClosedPtInCurve(polyline, point);
    const unsigned int vertexCount = polyline->numVerts();
    if (vertexCount < 2U) {
        return false;
    }

    AcGePoint3dArray vertices;  // 顶点集合
    AcArray<double> bulges;     // 凸度集合(每段对应一个 bulge)

    const bool isClosed = KTArxEntity::IsClosedPline(polyline);
    const bool isNativeClosed = polyline->isClosed();
    for (unsigned int i = 0; i < vertexCount; i++) {
        AcGePoint3d vertex;
        polyline->getPointAt(i, vertex);
        vertices.append(vertex);

        const bool isLastVertex = (i == (vertexCount - 1U));
        if (isClosed && !isNativeClosed && isLastVertex) {
            continue;
        }
        if (!isClosed && isLastVertex) {
            continue;
        }

        double bulgeValue = 0.0;
        polyline->getBulgeAt(i, bulgeValue);
        bulges.append(bulgeValue);
    }

    bool isVertex = false;
    int vertexIndex = -1;
    for (int i = 0; i < vertices.length(); i++) {
        if (pointOnCurve.isEqualTo(vertices[i])) {
            isVertex = true;
            vertexIndex = i;
            break;
        }
    }

    if (!isVertex) {
        AcGeVector3d deriv;
        polyline->getFirstDeriv(pointOnCurve, deriv);
        tangentVectors.append(deriv);
        tangentVectors.append(deriv.negate());
        return true;
    }

    const int segmentCount = bulges.length();
    if (segmentCount < 1) {
        return false;
    }

    const auto segmentEndVertexIndex = [&](int segmentStartIndex) -> int {
        if (isClosed && isNativeClosed && segmentStartIndex == (segmentCount - 1)) {
            return 0;
        }
        return segmentStartIndex + 1;
    };

    const auto getTangentAtVertexForSegment = [&](int segmentStartIndex, bool atStartVertex) -> AcGeVector3d {
        const int endIndex = segmentEndVertexIndex(segmentStartIndex);
        const double bulgeValue = bulges[segmentStartIndex];

        if (bulgeValue == 0.0) {
            if (atStartVertex) {
                return vertices[endIndex] - vertices[segmentStartIndex];
            }
            return vertices[segmentStartIndex] - vertices[endIndex];
        }

        AcGeCircArc2d segmentArc;
        polyline->getArcSegAt(static_cast<unsigned int>(segmentStartIndex), segmentArc);
        const AcGePoint3d centerPoint = KTArxConvert::ToPt3d(segmentArc.center());

        const AcGePoint3d& vertexPoint = atStartVertex ? vertices[segmentStartIndex] : vertices[endIndex];
        const AcGeVector3d radiusVector = vertexPoint - centerPoint;
        AcGeVector3d tangent = radiusVector.perpVector();

        const bool isPositiveBulge = bulgeValue > 0.0;
        if (atStartVertex) {
            if (!isPositiveBulge) {
                tangent.negate();
            }
            return tangent;
        }

        if (isPositiveBulge) {
            tangent.negate();
        }
        return tangent;
    };

    const int lastVertexIndex = vertices.length() - 1;

    if (isClosed) {
        if (vertexIndex == 0) {
            tangentVectors.append(getTangentAtVertexForSegment(0, true));
            tangentVectors.append(getTangentAtVertexForSegment(segmentCount - 1, false));
            return true;
        }

        tangentVectors.append(getTangentAtVertexForSegment(vertexIndex, true));
        tangentVectors.append(getTangentAtVertexForSegment(vertexIndex - 1, false));
        return true;
    }

    if (vertexIndex == 0) {
        tangentVectors.append(getTangentAtVertexForSegment(0, true));
        return true;
    }
    if (vertexIndex == lastVertexIndex) {
        tangentVectors.append(getTangentAtVertexForSegment(vertexIndex - 1, false));
        return true;
    }

    tangentVectors.append(getTangentAtVertexForSegment(vertexIndex, true));
    tangentVectors.append(getTangentAtVertexForSegment(vertexIndex - 1, false));
    return true;
}

AcGeVector3dArray KTArxCurve::GetTangentLineVecArr(const AcGePoint3d& point, AcDbCurve* curve) {
    constexpr double kEqualPointTol = 0.01;
    AcGeTol tol;
    tol.setEqualPoint(kEqualPointTol);

    AcGeVector3dArray tangentVectors;
    if (curve == nullptr) {
        return tangentVectors;
    }

    const AcGePoint3d pointOnCurve = GetClosedPtInCurve(curve, point);

    if (curve->isKindOf(AcDbCircle::desc()) || curve->isKindOf(AcDbEllipse::desc())) {  // 圆、椭圆
        AcGeVector3d vecDeriv;
        curve->getFirstDeriv(pointOnCurve, vecDeriv);
        tangentVectors.append(vecDeriv);
        tangentVectors.append(vecDeriv.negate());
        return tangentVectors;
    }

    if (curve->isKindOf(AcDbLine::desc()) || curve->isKindOf(AcDbArc::desc())) {  // 直线、圆弧
        AcGeVector3d vecDeriv;
        curve->getFirstDeriv(pointOnCurve, vecDeriv);

        AcGePoint3d ptStart, ptEnd;
        curve->getStartPoint(ptStart);
        curve->getEndPoint(ptEnd);

        if (pointOnCurve.isEqualTo(ptStart, tol)) {
            tangentVectors.append(vecDeriv);
            return tangentVectors;
        }
        if (pointOnCurve.isEqualTo(ptEnd, tol)) {
            tangentVectors.append(vecDeriv.negate());
            return tangentVectors;
        }

        tangentVectors.append(vecDeriv);
        tangentVectors.append(vecDeriv.negate());
        return tangentVectors;
    }

    if (curve->isKindOf(AcDbPolyline::desc())) {
        (void)GetTangentLineVecArrForPolyline(AcDbPolyline::cast(curve), point, tangentVectors);
        return tangentVectors;
    }

    return tangentVectors;
}

AcGeVector3dArray KTArxCurve::GetTangentLineVecArr(const AcGePoint3d& point, const AcDbObjectId& curveId) {
    AcDbObjectPointer<AcDbCurve> curve(curveId, AcDb::kForRead);
    return GetTangentLineVecArr(point, curve);
}

AcGeVector2d KTArxCurve::GetMidVectorOfLines(AcDbLine* pLine1, AcDbLine* pLine2) {
    AcGeLine2d geLine1 = GetGeLine2d(pLine1);
    AcGeLine2d geLine2 = GetGeLine2d(pLine2);
    AcGePoint3d ptInter;
    GetIntersectPoint(geLine1, geLine2, ptInter);
    AcGePoint3d ptEnd1 = ptInter.isEqualTo(pLine1->startPoint()) ? pLine1->endPoint() : pLine1->startPoint();
    AcGePoint3d ptEnd2 = ptInter.isEqualTo(pLine2->startPoint()) ? pLine2->endPoint() : pLine2->startPoint();
    AcGeVector2d vec1 = ptEnd1.convert2d(AcGePlane::kXYPlane) - ptInter.convert2d(AcGePlane::kXYPlane);
    AcGeVector2d vec2 = ptEnd2.convert2d(AcGePlane::kXYPlane) - ptInter.convert2d(AcGePlane::kXYPlane);
    AcGeVector2d midVector = vec1.normal() + vec2.normal();
    return midVector;
}

double KTArxCurve::GetAngle(const AcGeVector3d& vector) { return AcGeVector3d::kXAxis.angleTo(vector, AcGeVector3d::kZAxis); }

double KTArxCurve::GetCurveLength(AcDbCurve* curve) {
    double length = 0.0;
    double endParam = 0.0;
    curve->getEndParam(endParam);
    curve->getDistAtParam(endParam, length);
    return length;
}
}  // namespace KTArxTool

double KTArxTool::KTArxCurve::GetAngleOfLines(AcDbLine* pLine1, AcDbLine* pLine2) {
    AcGeLine2d geLine1 = GetGeLine2d(pLine1);
    AcGeLine2d geLine2 = GetGeLine2d(pLine2);
    AcGePoint3d ptInter;
    if (!GetIntersectPoint(geLine1, geLine2, ptInter)) return 0.;
    AcGePoint3d ptEnd1 = ptInter.isEqualTo(pLine1->startPoint()) ? pLine1->endPoint() : pLine1->startPoint();
    AcGePoint3d ptEnd2 = ptInter.isEqualTo(pLine2->startPoint()) ? pLine2->endPoint() : pLine2->startPoint();
    AcGeVector2d vec1 = ptEnd1.convert2d(AcGePlane::kXYPlane) - ptInter.convert2d(AcGePlane::kXYPlane);
    AcGeVector2d vec2 = ptEnd2.convert2d(AcGePlane::kXYPlane) - ptInter.convert2d(AcGePlane::kXYPlane);
    return vec1.angleTo(vec2);
}
