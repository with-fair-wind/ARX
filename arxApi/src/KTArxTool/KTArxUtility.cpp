namespace KTArxTool
{
    CString KTArxUtility::GetCurZrxFilePath()
    {
        TCHAR szPath[_MAX_PATH];
        ::GetModuleFileName(_hdllInstance, szPath, _MAX_PATH);
        return szPath;
    }

    CString KTArxUtility::GetCurZrxFolderPath()
    {
        TCHAR szPath[_MAX_PATH];
        ::GetModuleFileName(_hdllInstance, szPath, _MAX_PATH);
        CString strZrxPath = szPath;
        int nIndex = strZrxPath.ReverseFind(_T('\\'));
        CString strFloder = strZrxPath.Left(nIndex + 1);
        return strFloder;
    }

    bool KTArxUtility::LoadCadMenu(const CString &strMenuFileName, const CString &strMenuGroupName)
    {
        CString strFilePath = strMenuFileName;
        CFileStatus st;
        if (!CFile::GetStatus(strFilePath, st))
            return false;
        if (strMenuGroupName.GetLength() < 1 || strFilePath.GetLength() < 1)
            return false;
        CString strCmdLoad;
        strFilePath.Replace(_T("\\"), _T("\\\\"));
        if (!strMenuGroupName.IsEmpty() && strFilePath.GetLength() > 0)
            strCmdLoad.Format(_T("(if (null (menugroup \"%s\")) (command \"menuload\" \"%s\"))"), strMenuGroupName, strFilePath);
        else
            strCmdLoad.Format(_T("(command \"menuload\" \"%s\")"), strFilePath);
        int erStat = ads_queueexpr((LPTSTR)(LPCTSTR)strCmdLoad); // 此函数，相当于程序结束后，给CAD发送一个加载菜单的命令
        return true;
    }

    bool KTArxUtility::UnloadCadMenu(const CString &strMenuGroupName)
    {
        CString strCmdLoad;
        strCmdLoad.Format(_T("(command \"menuunload\" \"%s\")"), strMenuGroupName);
        int erStat = ads_queueexpr((LPTSTR)(LPCTSTR)strCmdLoad);
        return true;
    }

    bool KTArxUtility::LoadZrxFile(LPCTSTR szFilePath)
    {
        bool bRet = false;
        CFileStatus st;
        if (!CFile::GetStatus(szFilePath, st))
            return false;
        try
        {
            bRet = acrxLoadModule(st.m_szFullName, true, false);
        }
        catch (...)
        {
            bRet = false;
        }
        return bRet;
    }

    bool KTArxUtility::AddSearchPath(LPCTSTR szSearchDir)
    {
        CFileStatus st;
        if (!CFile::GetStatus(szSearchDir, st) || !(st.m_attribute & CFile::directory))
            return false;

        CString strSumPath;
        TCHAR szValue[1024];
        memset(szValue, 0, 1024 * sizeof(TCHAR));
        acedGetEnv(_T("ZWCAD"), szValue); // ACAD
        strSumPath = szValue;
        if (strSumPath.GetAt(strSumPath.GetLength() - 1) != _T(';'))
            strSumPath += _T(';');

        strSumPath = strSumPath + szSearchDir + _T(';');
        acedSetEnv(_T("ZWCAD"), strSumPath); // ACAD

        return true;
    }

    AcDbObjectId KTArxUtility::PostToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb /*= acdbCurDwg()*/)
    {
        assert(pEnt);
#if 0
        AcDbBlockTable *pBlkTbl = nullptr;
        pDb->getBlockTable(pBlkTbl, AcDb::kForRead);
        AcDbBlockTableRecord *pBlkTblRcd = nullptr;
        pBlkTbl->getAt(ACDB_MODEL_SPACE, pBlkTblRcd, AcDb::kForWrite);
        pBlkTbl->close();

        AcDbObjectId entId = AcDbObjectId::kNull;
        Acad::ErrorStatus es = pBlkTblRcd->appendAcDbEntity(entId, pEnt);
        if (es != Acad::eOk)
        {
            pBlkTblRcd->close();
            acutPrintf(_T("添加失败"));
            delete pEnt; // 添加失败时，要delete;
            pEnt = nullptr;

            return AcDbObjectId::kNull;
        }

        pBlkTblRcd->close();
        pEnt->close();

        return entId;
#else
        AcDbBlockTableRecordPointer pBlkTblRec(pDb->currentSpaceId(), AcDb::kForWrite);
        if (pBlkTblRec.openStatus() != Acad::eOk)
            return AcDbObjectId::kNull;
        AcDbObjectId entId = AcDbObjectId::kNull;
        if (Acad::eOk != pBlkTblRec->appendAcDbEntity(entId, pEnt))
        {
            delete pEnt;
            pEnt = nullptr;
            return AcDbObjectId::kNull;
        }
        pEnt->close();
        return entId;
#endif
    }

    bool KTArxUtility::ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase)
    {
        AcGeMatrix3d mtx;
        mtx.setToScaling(dScale, ptBase);
        return pEnt->transformBy(mtx) == Acad::eOk;
    }

    bool KTArxUtility::ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase)
    {
        AcDbEntityPointer pEnt(idEnt, AcDb::kForWrite);
        if (pEnt.openStatus() != Acad::eOk)
            return false;
        return ModifyEntByScale(pEnt, dScale, ptBase);
    }

    bool KTArxUtility::ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale,
                                        const AcGePoint3d &ptBase)
    {
        bool flag = true;
        for (auto &cur : arrpEnt)
            if (flag = ModifyEntByScale(cur, dScale, ptBase); !flag)
                break;
        return flag;
    }

    AcArray<AcDbEntity *> KTArxUtility::ExplodeEnt(AcDbEntity *pEnt)
    {
        AcArray<AcDbEntity *> arrExpEnt;
        AcDbVoidPtrArray exArr;
        if (pEnt->explode(exArr) == Acad::eOk)
        {
            for (int i = 0; i < exArr.length(); i++)
                arrExpEnt.append(static_cast<AcDbEntity *>(exArr.at(i)));
        }
        return arrExpEnt;
    }

    AcArray<AcDbEntity *> KTArxUtility::ExplodeEnt(const AcDbObjectId &idEnt)
    {
        AcArray<AcDbEntity *> arrExpEnt;
        AcDbEntityPointer pEnt(idEnt, AcDb::kForRead);
        if (Acad::eOk != pEnt.openStatus())
            return arrExpEnt;
        return ExplodeEnt(pEnt);
    }

    bool KTArxUtility::MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        assert(pEnt);
        AcGeMatrix3d xform;
        AcGeVector3d vec(ptDest - ptBase);
        xform.setToTranslation(vec);
        return pEnt->transformBy(xform) == Acad::eOk;
    }

    bool KTArxUtility::MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        bool flag = true;
        for (auto &pEnt : arrEnt)
        {
            if (!pEnt)
                continue;
            if (flag = MoveEnt(pEnt, ptBase, ptDest); !flag)
                break;
        }
        return flag;
    }

    bool KTArxUtility::MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        AcDbEntityPointer pEnt(idEnt, AcDb::kForWrite);
        if (Acad::eOk != pEnt.openStatus())
            return false;
        return MoveEnt(pEnt, ptBase, ptDest);
    }

    bool KTArxUtility::MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        bool flag = true;
        for (int i = 0; i < arrid.length(); i++)
        {
            AcDbEntityPointer pEnt(arrid[i], AcDb::kForWrite);
            if (Acad::eOk != pEnt.openStatus())
                continue;
            if (flag = MoveEnt(pEnt, ptBase, ptDest); !flag)
                break;
        }
        return flag;
    }

    bool KTArxUtility::RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation)
    {
        assert(pEnt);
        AcGeMatrix3d xform;
        xform.setToRotation(rotation, AcGeVector3d::kZAxis, ptBase); // 参数二是输入旋转轴向量
        return pEnt->transformBy(xform) == Acad::eOk;
    }

    bool KTArxUtility::RotateEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, double rotation)
    {
        bool flag = true;
        for (auto &pEnt : arrEnt)
        {
            if (!pEnt)
                continue;
            if (flag = RotateEnt(pEnt, ptBase, rotation); !flag)
                break;
        }
        return flag;
    }

    bool KTArxUtility::GetPosAngNegValue(AcDbCurve *pCurve, const AcGePoint3d &ptBase,
                                         const AcGeVector3d &vecDir)
    {
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
        if (!bCodirectional)
            bFlag = !bFlag;
        // 如果是直线，取反
        if (pCurve->isKindOf(AcDbLine::desc()))
            bFlag = !bFlag;

        return bFlag;
    }

    AcArray<AcDbCurve *> KTArxUtility::OffsetCurve(AcDbCurve *pCurve, double dOffset)
    {
        AcArray<AcDbCurve *> arrOffsetCurve;
        AcDbVoidPtrArray osArr;
        if (pCurve->getOffsetCurves(dOffset, osArr) == Acad::eOk)
        {
            for (int i = 0; i < osArr.length(); ++i)
                arrOffsetCurve.append(static_cast<AcDbCurve *>(osArr.at(i)));
        }
        return arrOffsetCurve;
    }

    AcArray<AcDbCurve *> KTArxUtility::OffsetCurve(AcDbCurve *pCurve, double dOffset, const AcGePoint3d &pt, bool bOpposite /*= false*/)
    {
        dOffset = std::fabs(dOffset); // 绝对值
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
        if (!bCodirectional)
            dOffset = -dOffset;
        // 如果是直线，取反
        if (pCurve->isKindOf(AcDbLine::desc()))
            dOffset = -dOffset;
        if (bOpposite)
            dOffset = -dOffset;
        // 偏移实体
        return OffsetCurve(pCurve, dOffset);
    }

    AcGePoint3d KTArxUtility::GetClosedPtInCurve(AcDbCurve *pCurve, const AcGePoint3d &pt)
    {
        double dLength = 0.;
        if (Acad::eInvalidInput == pCurve->getDistAtPoint(pt, dLength))
        {
            AcGePoint3d ptOnCurve;
            pCurve->getClosestPointTo(pt, ptOnCurve);
            return ptOnCurve;
        }
        else
            return pt;
    }

    bool KTArxUtility::GetClosedPtInCurve(const AcDbObjectId &idCurve, const AcGePoint3d &pt, AcGePoint3d &ptClosed)
    {
        AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
        if (Acad::eOk != pCurve.openStatus())
            return false;
        ptClosed = GetClosedPtInCurve(pCurve, pt);
        return true;
    }

    AcArray<AcDbCurve *> KTArxUtility::SplitCurve(AcDbCurve *pCurve, const AcGePoint3dArray &arrSplitPt)
    {
        AcArray<AcDbCurve *> arrSplitCurve;
        AcDbVoidPtrArray spArr;
        AcGePoint3dArray arrptClosed;
        for (int i = 0; i < arrSplitPt.length(); i++)
            arrptClosed.append(GetClosedPtInCurve(pCurve, arrSplitPt.at(i)));

        if (pCurve->getSplitCurves(arrptClosed, spArr) == Acad::eOk)
        {
            for (int i = 0; i < spArr.length(); i++)
                arrSplitCurve.append(static_cast<AcDbCurve *>(spArr.at(i)));
        }
        return arrSplitCurve;
    }

    AcArray<AcDbCurve *> KTArxUtility::SplitCurve(const AcDbObjectId &idCurve, const AcGePoint3dArray &arrSplitPt)
    {
        AcArray<AcDbCurve *> arrpSplit;
        AcDbObjectPointer<AcDbCurve> pCurve(idCurve, AcDb::kForRead);
        if (Acad::eOk != pCurve.openStatus())
            return arrpSplit;
        return SplitCurve(pCurve, arrSplitPt);
    }

    AcArray<AcDbCurve *> KTArxUtility::SplitCurve(AcDbCurve *pCurve, const AcGePoint3d &ptSplit)
    {
        AcGePoint3dArray arrSplitPt;
        arrSplitPt.append(ptSplit);
        return SplitCurve(pCurve, arrSplitPt);
    }

    bool KTArxUtility::ExtendCurveByDist(AcDbPolyline *pPline, bool isStartPt,
                                         double dDist)
    {
        if (pPline->numVerts() < 2)
            return false;
        if (isStartPt)
        {
            AcGePoint3d sPt, sPt2;
            pPline->getStartPoint(sPt);
            pPline->getPointAt(1, sPt2);
            double td1, td2;
            pPline->getDistAtPoint(sPt, td1);
            pPline->getDistAtPoint(sPt2, td2);
            double off = td2 - td1;
            double sParam = fabs(dDist) / off;
            return pPline->extend(-sParam) == Acad::eOk;
        }
        else
        {
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

    bool KTArxUtility::ExtendCurve(AcDbCurve *pBorder, AcDbCurve *pExtend, const AcGePoint3d &ptExtend)
    {
        AcGePoint3dArray arrptInter;
        pExtend->intersectWith(pBorder, AcDb::kExtendThis, arrptInter);
        if (arrptInter.isEmpty())
            return false;
        AcGePoint3d ptStart, ptEnd;
        pExtend->getStartPoint(ptStart);
        pExtend->getEndPoint(ptEnd);
        double dDistStart = ptExtend.distanceTo(ptStart);
        double dDistEnd = ptExtend.distanceTo(ptEnd);
        return pExtend->extend(dDistStart < dDistEnd, arrptInter[0]);
    }

    bool KTArxUtility::TrimCurve(AcDbCurve *pBorder, AcDbCurve *pTrim, const AcGePoint3d &ptTrim, AcDbCurve *&pNewTrim)
    {
        AcGePoint3dArray arrptInter;
        pTrim->intersectWith(pBorder, AcDb::kOnBothOperands, arrptInter);
        if (arrptInter.isEmpty())
            return false;

        AcArray<AcDbCurve *> arrSplit = SplitCurve(pTrim, arrptInter[0]);
        if (arrSplit.length() != 2)
        {
            for (int i = 0; i < arrSplit.length(); i++)
                DEL(arrSplit[i]);
            return false;
        }

        if (IsPtOnCurve(ptTrim, arrSplit[0]))
        {
            pNewTrim = arrSplit[1];
            DEL(arrSplit[0]);
            return true;
        }
        else if (IsPtOnCurve(ptTrim, arrSplit[1]))
        {
            pNewTrim = arrSplit[0];
            DEL(arrSplit[1]);
            return true;
        }
        else
        {
            for (int i = 0; i < arrSplit.length(); i++)
                DEL(arrSplit[i]);
            return false;
        }
    }

    bool KTArxUtility::IsPtOnCurve(const AcGePoint3d &pt, AcDbCurve *pCurve, double dTol /*= 0*/)
    {
        double dLength = 0;
        if (Acad::eInvalidInput != pCurve->getDistAtPoint(pt, dLength))
            return true;
        else
        {
            if (!dTol)
                return false;
            else
            {
                AcGePoint3d ptOnCurve = GetClosedPtInCurve(pCurve, pt);
                AcGeTol tol;
                tol.setEqualPoint(dTol);
                return pt.isEqualTo(ptOnCurve, tol);
            }
        }
    }
} // namespace KK
