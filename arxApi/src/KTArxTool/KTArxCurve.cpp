#include <KTArxTool/KTArxConvert.h>
#include <KTArxTool/KTArxCurve.h>
#include <KTArxTool/KTArxEntity.h>

namespace KTArxTool {
bool KTArxCurve::GetPosAngNegValue(AcDbCurve* pCurve, const AcGePoint3d& ptBase, const AcGeVector3d& vecDir) {
    AcGePoint3d ptNew = ptBase + vecDir;
    bool bFlag = true;

    // 算出实体平面法向量
    AcGePlane plane;
    AcDb::Planarity type;
    pCurve->getPlane(plane, type);
    AcGeVector3d vecNormalNew = plane.normal();
    // 根据偏移点（鼠标点），找出矩形上距离该点最近的点ptClosed
    AcGePoint3d ptClosed;
    pCurve->getClosestPointTo(ptNew, ptClosed);
    // 算出矩形上经过ptClosed的切线，即经过该点的那条边vecDerive
    AcGeVector3d vecDerive;
    pCurve->getFirstDeriv(ptClosed, vecDerive);
    // 使用ptClosed得到一个偏移向量vecOffsetdir
    AcGeVector3d vecOffsetdir = ptNew - ptClosed;
    // vecOffset和vecDerive做叉乘，算出法向量vecOffsetNormal，看vecOffsetNormal和
    // vecNormal是否同向，如果同向偏移距离为dOffset，如果不同向偏移距离为-dOffset
    AcGeVector3d vecOffsetNormal = vecOffsetdir.crossProduct(vecDerive);
    bool bCodirectional = vecOffsetNormal.isCodirectionalTo(vecNormalNew);
    if (!bCodirectional) bFlag = !bFlag;
    // 如果是直线，取反
    if (pCurve->isKindOf(AcDbLine::desc())) bFlag = !bFlag;

    return bFlag;
}

AcArray<AcDbCurve*> KTArxCurve::OffsetCurve(AcDbCurve* pCurve, double dOffset) {
    AcArray<AcDbCurve*> arrOffsetCurve;
    AcDbVoidPtrArray osArr;
    if (pCurve->getOffsetCurves(dOffset, osArr) == Acad::eOk) {
        for (int i = 0; i < osArr.length(); ++i) arrOffsetCurve.append(static_cast<AcDbCurve*>(osArr.at(i)));
    }
    return arrOffsetCurve;
}

AcArray<AcDbCurve*> KTArxCurve::OffsetCurve(AcDbCurve* pCurve, double dOffset, const AcGePoint3d& pt, bool bOpposite /*= false*/) {
    dOffset = std::fabs(dOffset);  // 绝对值
    // 算出实体平面法向量
    AcGePlane plane;
    AcDb::Planarity type;
    pCurve->getPlane(plane, type);
    AcGeVector3d vecNormal = plane.normal();
    // 根据偏移点（鼠标点），找出矩形上距离该点最近的点ptClosed
    AcGePoint3d ptClosed = GetClosedPtInCurve(pCurve, pt);
    // 算出矩形上经过ptClosed的切线，即经过该点的那条边vecDerive
    AcGeVector3d vecDerive;
    pCurve->getFirstDeriv(ptClosed, vecDerive);
    // 使用ptClosed得到一个偏移向量vecOffsetdir
    AcGeVector3d vecOffsetdir = pt - ptClosed;
    // vecOffset和vecDerive做叉乘，算出法向量vecOffsetNormal，看vecOffsetNormal和
    // vecNormal是否同向，如果同向偏移距离为dOffset，如果不同向偏移距离为-dOffset
    AcGeVector3d vecOffsetNormal = vecOffsetdir.crossProduct(vecDerive);
    bool bCodirectional = vecOffsetNormal.isCodirectionalTo(vecNormal);
    if (!bCodirectional) dOffset = -dOffset;
    // 如果是直线，取反
    if (pCurve->isKindOf(AcDbLine::desc())) dOffset = -dOffset;
    if (bOpposite) dOffset = -dOffset;
    // 偏移实体
    return OffsetCurve(pCurve, dOffset);
}

AcGePoint3d KTArxCurve::GetClosedPtInCurve(AcDbCurve* pCurve, const AcGePoint3d& pt) {
    double dLength = 0.;
    if (Acad::eInvalidInput == pCurve->getDistAtPoint(pt, dLength)) {
        AcGePoint3d ptOnCurve;
        pCurve->getClosestPointTo(pt, ptOnCurve);
        return ptOnCurve;
    } else
        return pt;
}

bool KTArxCurve::GetClosedPtInCurve(const AcDbObjectId& idCurve, const AcGePoint3d& pt, AcGePoint3d& ptClosed) {
    AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
    if (Acad::eOk != pCurve.openStatus()) return false;
    ptClosed = GetClosedPtInCurve(pCurve, pt);
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

bool KTArxCurve::ExtendCurveByDist(AcDbPolyline* pPline, bool isStartPt, double dDist) {
    if (pPline->numVerts() < 2) return false;
    if (isStartPt) {
        AcGePoint3d sPt, sPt2;
        pPline->getStartPoint(sPt);
        pPline->getPointAt(1, sPt2);
        double td1, td2;
        pPline->getDistAtPoint(sPt, td1);
        pPline->getDistAtPoint(sPt2, td2);
        double off = td2 - td1;
        double sParam = fabs(dDist) / off;
        return pPline->extend(-sParam) == Acad::eOk;
    } else {
        AcGePoint3d ePt, ePt2;
        pPline->getEndPoint(ePt);
        pPline->getPointAt(pPline->numVerts() - 2, ePt2);
        double td1, td2;
        pPline->getDistAtPoint(ePt2, td1);
        pPline->getDistAtPoint(ePt, td2);
        double off = td2 - td1;
        double eparam = fabs(dDist) / off;
        return pPline->extend(eparam) == Acad::eOk;
    }
}

bool KTArxCurve::ExtendCurve(AcDbCurve* pBorder, AcDbCurve* pExtend, const AcGePoint3d& ptExtend) {
    AcGePoint3dArray arrptInter;
    pExtend->intersectWith(pBorder, AcDb::kExtendThis, arrptInter);
    if (arrptInter.isEmpty()) return false;
    AcGePoint3d ptStart, ptEnd;
    pExtend->getStartPoint(ptStart);
    pExtend->getEndPoint(ptEnd);
    double dDistStart = ptExtend.distanceTo(ptStart);
    double dDistEnd = ptExtend.distanceTo(ptEnd);
    return pExtend->extend(dDistStart < dDistEnd, arrptInter[0]);
}

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
    } else if (IsPtOnCurve(ptTrim, arrSplit[1])) {
        pNewTrim = arrSplit[0];
        DEL(arrSplit[1]);
        return true;
    } else {
        for (int i = 0; i < arrSplit.length(); i++) DEL(arrSplit[i]);
        return false;
    }
}

bool KTArxCurve::IsPtOnCurve(const AcGePoint3d& pt, AcDbCurve* pCurve, double dTol /*= 0*/) {
    double dLength = 0;
    if (Acad::eInvalidInput != pCurve->getDistAtPoint(pt, dLength))
        return true;
    else {
        if (!dTol)
            return false;
        else {
            AcGePoint3d ptOnCurve = GetClosedPtInCurve(pCurve, pt);
            AcGeTol tol;
            tol.setEqualPoint(dTol);
            return pt.isEqualTo(ptOnCurve, tol);
        }
    }
}

double KTArxCurve::GetArcBulge(AcDbArc* pArc) { return GetArcBulge(pArc->startAngle(), pArc->endAngle()); }

double KTArxCurve::GetArcBulge(double dAngleStart, double dAngleEnd) {
    double dAlfa = dAngleEnd - dAngleStart;
    if (dAlfa < 0.0)  // 如果终点角度小于起点角度;
        dAlfa = 2 * (atan(1.0) * 4) + dAlfa;
    return tan((dAlfa) / 4.0);
}

double KTArxCurve::GetArcBulge(const AcGeCircArc2d& geArc) { return GetArcBulge(geArc.startAng(), geArc.endAng()); }

double KTArxCurve::GetArcBulge(AcDbCurve* pCurve) {
    double dBulge = 0.0;
    if (pCurve->isKindOf(AcDbArc::desc())) {
        AcDbArc* pArc = AcDbArc::cast(pCurve);
        dBulge = GetArcBulge(pArc->startAngle(), pArc->endAngle());
    }
    return dBulge;
}

double KTArxCurve::GetArcBulge(const AcDbObjectId& idArc) {
    AcDbObjectPointer<AcDbArc> pArc(idArc, AcDb::kForRead);
    if (Acad::eOk != pArc.openStatus()) return 0;
    return GetArcBulge(pArc->startAngle(), pArc->endAngle());
}

AcGeLine2d KTArxCurve::GetGeLine2d(AcDbLine* pLine) {
    AcGePoint3d ptStart = pLine->startPoint();
    AcGePoint3d ptEnd = pLine->endPoint();
    AcGeLine2d geLine;
    geLine.set(ptStart.convert2d(AcGePlane::kXYPlane), ptEnd.convert2d(AcGePlane::kXYPlane));
    return geLine;
}

AcGeLineSeg2d KTArxCurve::GetGeLineSeg2d(AcDbLine* pLine) {
    AcGePoint3d ptStart = pLine->startPoint();
    AcGePoint3d ptEnd = pLine->endPoint();
    AcGeLineSeg2d geLine;
    geLine.set(ptStart.convert2d(AcGePlane::kXYPlane), ptEnd.convert2d(AcGePlane::kXYPlane));
    return geLine;
}

AcGeLineSeg2d KTArxCurve::GetGeLineSeg2d(AcDbPolyline* pPolyline, unsigned int unIndex) {
    AcGeLineSeg2d geLine;
    Acad::ErrorStatus es = pPolyline->getLineSegAt(unIndex, geLine);
    return geLine;
}

AcGeLineSeg2d KTArxCurve::GetGeLineSeg2d(const AcDbObjectId& idLine) {
    AcGeLineSeg2d geLine;
    AcDbObjectPointer<AcDbLine> pLine(idLine, AcDb::kForRead);
    if (Acad::eOk != pLine.openStatus()) return geLine;
    AcGePoint3d ptStart = pLine->startPoint();
    AcGePoint3d ptEnd = pLine->endPoint();
    geLine.set(ptStart.convert2d(AcGePlane::kXYPlane), ptEnd.convert2d(AcGePlane::kXYPlane));
    return geLine;
}

AcGeCircArc2d KTArxCurve::GetGeCircArc2d(AcDbPolyline* pPolyline, unsigned int unIndex) {
    AcGeCircArc2d geCircArc;
    Acad::ErrorStatus es = pPolyline->getArcSegAt(unIndex, geCircArc);
    return geCircArc;
}

AcGeCircArc2d KTArxCurve::GetGeCircArc2d(AcDbArc* pArc) {
    AcGePoint3d centerPt = pArc->center();
    double dRadius = pArc->radius();
    double dStartAngle = pArc->startAngle();
    double dEndAngle = pArc->endAngle();
    AcGeCircArc2d geCircArc;
    geCircArc.set(centerPt.convert2d(AcGePlane::kXYPlane), dRadius, dStartAngle, dEndAngle);
    return geCircArc;
}

AcGeCircArc2d KTArxCurve::GetGeCircArc2d(AcDbCircle* pCircle) {
    AcGePoint3d centerPt = pCircle->center();
    double dRadius = pCircle->radius();
    AcGeCircArc2d geCircArc;
    geCircArc.set(centerPt.convert2d(AcGePlane::kXYPlane), dRadius);
    return geCircArc;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLine2d& geLine1, const AcGeLine2d& geLine2, AcGePoint3d& ptIntersect) {
    AcGePoint2d pt2d;
    bool bRet = geLine1.intersectWith(geLine2, pt2d);
    if (!bRet) return false;
    ptIntersect = AcGePoint3d(pt2d.x, pt2d.y, 0);
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLineSeg2d& geLine1, const AcGeLineSeg2d& geLine2, AcGePoint3d& ptIntersect) {
    AcGePoint2d pt2d;
    bool bRet = geLine1.intersectWith(geLine2, pt2d);
    if (!bRet) return false;
    ptIntersect = AcGePoint3d(pt2d.x, pt2d.y, 0);
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLineSeg2d& geLine1, const AcGeLine2d& geLine2, AcGePoint3d& ptIntersect) {
    AcGePoint2d pt2d;
    bool bRet = geLine1.intersectWith(geLine2, pt2d);
    if (!bRet) return false;
    ptIntersect = AcGePoint3d(pt2d.x, pt2d.y, 0);
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeCircArc2d& geCirArc1, const AcGeCircArc2d& geCirArc2, int& nNum, AcGePoint3dArray& arrptIntersect) {
    nNum = 0;
    AcGePoint2d pt1, pt2;
    bool bRet = geCirArc1.intersectWith(geCirArc2, nNum, pt1, pt2);
    if (!bRet) return false;
    if (1 == nNum)
        arrptIntersect.append(AcGePoint3d(pt1.x, pt1.y, 0));
    else if (2 == nNum) {
        arrptIntersect.append(AcGePoint3d(pt1.x, pt1.y, 0));
        arrptIntersect.append(AcGePoint3d(pt2.x, pt2.y, 0));
    }
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLine2d& geLine, const AcGeCircArc2d& geCircArc, int& nNum, AcGePoint3dArray& arrptIntersect) {
    nNum = 0;
    AcGePoint2d pt1, pt2;
    if (!geCircArc.intersectWith(geLine, nNum, pt1, pt2)) return false;
    if (nNum == 1)
        arrptIntersect.append(AcGePoint3d(pt1.x, pt1.y, 0));
    else if (nNum == 2) {
        arrptIntersect.append(AcGePoint3d(pt1.x, pt1.y, 0));
        arrptIntersect.append(AcGePoint3d(pt2.x, pt2.y, 0));
    }
    return true;
}

bool KTArxCurve::GetIntersectPoint(const AcGeLineSeg2d& geLine, const AcGeCircArc2d& geCircArc, int& nNum, AcGePoint3dArray& arrptIntersect) {
    nNum = 0;
    AcGePoint2d pt1, pt2;
    if (!geCircArc.intersectWith(geLine, nNum, pt1, pt2)) return false;
    if (nNum == 1)
        arrptIntersect.append(AcGePoint3d(pt1.x, pt1.y, 0));
    else if (nNum == 2) {
        arrptIntersect.append(AcGePoint3d(pt1.x, pt1.y, 0));
        arrptIntersect.append(AcGePoint3d(pt2.x, pt2.y, 0));
    }
    return true;
}

AcGeVector3d KTArxCurve::GetProjectVector(const AcGePoint3d& ptBase, const AcGeVector3d& vctOffset, const AcGeVector3d& vctDirection) {
    AcGePoint3d ptOffset = ptBase + vctOffset;
    AcGeLine3d geLine(ptBase, vctDirection);
    AcGePoint3d ptTarget = geLine.closestPointTo(ptOffset);
    return ptTarget - ptBase;
}

AcGeVector3d KTArxCurve::GetTangentLineVec(const AcGePoint3d& ptOnCurve, const AcDbObjectId& idCurve) {
    AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
    if (Acad::eOk != pCurve.openStatus()) return AcGeVector3d::kIdentity;
    AcGePoint3d pt = GetClosedPtInCurve(pCurve, ptOnCurve);
    AcGeVector3d vecTangLine;
    pCurve->getFirstDeriv(pt, vecTangLine);
    return vecTangLine;
}

bool KTArxCurve::GetTangentLineVecArrForPolyline(AcDbPolyline* pPline, const AcGePoint3d& pt, AcGeVector3dArray& arrvec) {
    AcGePoint3d ptOnCurve = GetClosedPtInCurve(pPline, pt);
    int nCountVerts = pPline->numVerts();
    if (nCountVerts < 2) return false;

    AcGePoint3dArray arrVertex;  // 顶点集合
    AcArray<double> arrBulge;    // 凸度集合
    bool bClosed = m_pArxEntity->IsClosedPline(pPline);
    bool bClosedFlag = pPline->isClosed();
    for (int i = 0; i < nCountVerts; i++) {
        AcGePoint3d ptTemp;
        pPline->getPointAt(i, ptTemp);
        arrVertex.append(ptTemp);
        if (bClosed && !bClosedFlag && (i == nCountVerts - 1)) continue;
        if (!bClosed && (i == nCountVerts - 1)) continue;
        double dBugle = 0.;
        pPline->getBulgeAt(i, dBugle);
        arrBulge.append(dBugle);
    }

    bool bIsVertex = false;  // 是否是端点
    int nIndexVertex = -1;   // 端点索引
    for (int i = 0; i < arrVertex.length(); i++) {
        if (ptOnCurve.isEqualTo(arrVertex[i])) {
            bIsVertex = true;
            nIndexVertex = i;
            break;
        }
    }
    if (!bIsVertex) {
        AcGeVector3d vecDeriv;
        pPline->getFirstDeriv(ptOnCurve, vecDeriv);
        arrvec.append(vecDeriv);
        arrvec.append(vecDeriv.negate());
        return true;
    } else {
        if (bClosed)  // 闭合线段
        {
            if (nIndexVertex == 0)  // 线段第一点
            {
                // 第一条方向
                if (arrBulge[nIndexVertex] == 0)  // 是直线
                {
                    AcGeVector3d vecDeriv = arrVertex[1] - arrVertex[0];
                    arrvec.append(vecDeriv);
                } else  // 不是直线
                {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex] > 0)
                        vecDeriv = vec.perpVector();  // 逆时针
                    else
                        vecDeriv = vec.perpVector().negate();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                // 第二条方向
                int nEndIndex = arrBulge.length() - 1;
                if (arrBulge[nEndIndex] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[nEndIndex] - arrVertex[0];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nEndIndex, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[0] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nEndIndex] > 0)
                        vecDeriv = vec.perpVector().negate();  // 逆时针
                    else
                        vecDeriv = vec.perpVector();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                return true;
            } else {
                // 第一条方向
                if (arrBulge[nIndexVertex] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[nIndexVertex + 1] - arrVertex[nIndexVertex];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex] > 0)
                        vecDeriv = vec.perpVector();  // 逆时针
                    else
                        vecDeriv = vec.perpVector().negate();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                // 第二条方向
                if (arrBulge[nIndexVertex - 1] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[nIndexVertex - 1] - arrVertex[nIndexVertex];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex - 1, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex - 1] > 0)
                        vecDeriv = vec.perpVector().negate();  // 逆时针
                    else
                        vecDeriv = vec.perpVector();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                return true;
            }
        } else  // 非闭合线段
        {
            if (nIndexVertex == 0) {
                if (arrBulge[nIndexVertex] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[1] - arrVertex[0];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex] > 0)
                        vecDeriv = vec.perpVector();  // 逆时针
                    else
                        vecDeriv = vec.perpVector().negate();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                return true;
            } else if (nIndexVertex == nCountVerts - 1) {
                if (arrBulge[nIndexVertex - 1] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[nIndexVertex - 1] - arrVertex[nIndexVertex];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex - 1, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex - 1] > 0)
                        vecDeriv = vec.perpVector().negate();  // 逆时针
                    else
                        vecDeriv = vec.perpVector();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                return true;
            } else {
                // 第一条方向
                if (arrBulge[nIndexVertex] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[nIndexVertex + 1] - arrVertex[nIndexVertex];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex] > 0)
                        vecDeriv = vec.perpVector();  // 逆时针
                    else
                        vecDeriv = vec.perpVector().negate();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                // 第二条方向
                if (arrBulge[nIndexVertex - 1] == 0) {
                    AcGeVector3d vecDeriv = arrVertex[nIndexVertex - 1] - arrVertex[nIndexVertex];
                    arrvec.append(vecDeriv);
                } else {
                    AcGeCircArc2d segArc;
                    pPline->getArcSegAt(nIndexVertex - 1, segArc);
                    AcGePoint3d ptCenter = m_pArxConvert->ToPt3d(segArc.center());
                    AcGeVector3d vec = arrVertex[nIndexVertex] - ptCenter;
                    AcGeVector3d vecDeriv;
                    if (arrBulge[nIndexVertex - 1] > 0)
                        vecDeriv = vec.perpVector().negate();  // 逆时针
                    else
                        vecDeriv = vec.perpVector();  // 顺时针
                    arrvec.append(vecDeriv);
                }
                return true;
            }
        }
    }
}

AcGeVector3dArray KTArxCurve::GetTangentLineVecArr(const AcGePoint3d& pt, AcDbCurve* pCurve) {
    // 公差
    double dTol = 0.01;
    AcGeTol tol;
    tol.setEqualPoint(dTol);
    //
    AcGeVector3dArray arrvec;
    AcGePoint3d ptOnCurve;
    // pCurve->getClosestPointTo(pt, ptOnCurve);
    ptOnCurve = GetClosedPtInCurve(pCurve, pt);

    if (pCurve->isKindOf(AcDbCircle::desc()) || pCurve->isKindOf(AcDbEllipse::desc())) {  // 圆、椭圆
        AcGeVector3d vecDeriv;
        pCurve->getFirstDeriv(ptOnCurve, vecDeriv);
        arrvec.append(vecDeriv);
        arrvec.append(vecDeriv.negate());
        return arrvec;
    } else if (pCurve->isKindOf(AcDbLine::desc()) || pCurve->isKindOf(AcDbArc::desc())) {  // 直线、圆弧
        AcGeVector3d vecDeriv;
        pCurve->getFirstDeriv(ptOnCurve, vecDeriv);
        AcGePoint3d ptStart, ptEnd;
        pCurve->getStartPoint(ptStart);
        pCurve->getEndPoint(ptEnd);
        if (ptOnCurve.isEqualTo(ptStart, tol)) {
            arrvec.append(vecDeriv);
            return arrvec;
        } else if (ptOnCurve.isEqualTo(ptEnd, tol)) {
            arrvec.append(vecDeriv.negate());
            return arrvec;
        } else {
            arrvec.append(vecDeriv);
            arrvec.append(vecDeriv.negate());
            return arrvec;
        }
    } else if (pCurve->isKindOf(AcDbPolyline::desc())) {
        bool success = GetTangentLineVecArrForPolyline(AcDbPolyline::cast(pCurve), pt, arrvec);
        return arrvec;
    } else
        return arrvec;
}

AcGeVector3dArray KTArxCurve::GetTangentLineVecArr(const AcGePoint3d& pt, const AcDbObjectId& idCurve) {
    AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
    return GetTangentLineVecArr(pt, pCurve);
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

double KTArxCurve::GetAngle(const AcGeVector3d& vec) { return AcGeVector3d::kXAxis.angleTo(vec, AcGeVector3d::kZAxis); }

double KTArxCurve::GetCurveLength(AcDbCurve* pCurve) {
    double dLength = 0;
    double endParam = 0;
    pCurve->getEndParam(endParam);
    pCurve->getDistAtParam(endParam, dLength);
    return dLength;
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
