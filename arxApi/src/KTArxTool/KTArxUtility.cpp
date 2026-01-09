namespace KTArxTool {
CString KTArxUtility::GetCurZrxFilePath() {
    TCHAR szPath[_MAX_PATH];
    ::GetModuleFileName(_hdllInstance, szPath, _MAX_PATH);
    return szPath;
}

CString KTArxUtility::GetCurZrxFolderPath() {
    TCHAR szPath[_MAX_PATH];
    ::GetModuleFileName(_hdllInstance, szPath, _MAX_PATH);
    CString strZrxPath = szPath;
    int nIndex = strZrxPath.ReverseFind(_T('\\'));
    CString strFloder = strZrxPath.Left(nIndex + 1);
    return strFloder;
}

AcArray<CString> KTArxUtility::GetAllDwgPath(LPCTSTR szFolderPath) {
    AcArray<CString> arrDwgPath;
    CFileFind finder;
    CString path;
    path.Format(_T("%s\\*.dwg"), szFolderPath);
    BOOL bWorking = finder.FindFile(path);
    while (bWorking) {
        bWorking = finder.FindNextFileW();
        if (finder.IsDirectory() || finder.IsDots()) continue;
        CString strDwgPath = finder.GetFilePath();
        arrDwgPath.append(strDwgPath);
    }
    return arrDwgPath;
}

AcArray<CString> KTArxUtility::GetAllDwgName(LPCTSTR szFolderPath) {
    AcArray<CString> arrDwgPath;
    CFileFind finder;
    CString path;
    path.Format(_T("%s\\*.dwg"), szFolderPath);
    BOOL bWorking = finder.FindFile(path);
    while (bWorking) {
        bWorking = finder.FindNextFileW();
        if (finder.IsDirectory() || finder.IsDots()) continue;
        CString strDwgName = finder.GetFileTitle();
        arrDwgPath.append(strDwgName);
    }
    return arrDwgPath;
}

bool KTArxUtility::LoadCadMenu(const CString& strMenuFileName, const CString& strMenuGroupName) {
    CString strFilePath = strMenuFileName;
    CFileStatus st;
    if (!CFile::GetStatus(strFilePath, st)) return false;
    if (strMenuGroupName.GetLength() < 1 || strFilePath.GetLength() < 1) return false;
    CString strCmdLoad;
    strFilePath.Replace(_T("\\"), _T("\\\\"));
    if (!strMenuGroupName.IsEmpty() && strFilePath.GetLength() > 0)
        strCmdLoad.Format(_T("(if (null (menugroup \"%s\")) (command \"menuload\" \"%s\"))"), (LPCTSTR)strMenuGroupName, (LPCTSTR)strFilePath);
    else
        strCmdLoad.Format(_T("(command \"menuload\" \"%s\")"), (LPCTSTR)strFilePath);
    int erStat = ads_queueexpr((LPTSTR)(LPCTSTR)strCmdLoad);  // 此函数，相当于程序结束后，给CAD发送一个加载菜单的命令
    return true;
}

bool KTArxUtility::UnloadCadMenu(const CString& strMenuGroupName) {
    CString strCmdLoad;
    strCmdLoad.Format(_T("(command \"menuunload\" \"%s\")"), (LPCTSTR)strMenuGroupName);
    int erStat = ads_queueexpr((LPTSTR)(LPCTSTR)strCmdLoad);
    return true;
}

bool KTArxUtility::LoadZrxFile(LPCTSTR szFilePath) {
    bool bRet = false;
    CFileStatus st;
    if (!CFile::GetStatus(szFilePath, st)) return false;
    try {
        bRet = acrxLoadModule(st.m_szFullName, true, false);
    } catch (...) {
        bRet = false;
    }
    return bRet;
}

bool KTArxUtility::AddSearchPath(LPCTSTR szSearchDir) {
    CFileStatus st;
    if (!CFile::GetStatus(szSearchDir, st) || !(st.m_attribute & CFile::directory)) return false;

    CString strSumPath;
    TCHAR szValue[1024];
    memset(szValue, 0, 1024 * sizeof(TCHAR));
    acedGetEnv(_T("ZWCAD"), szValue);  // ACAD
    strSumPath = szValue;
    if (strSumPath.GetAt(strSumPath.GetLength() - 1) != _T(';')) strSumPath += _T(';');

    strSumPath = strSumPath + szSearchDir + _T(';');
    acedSetEnv(_T("ZWCAD"), strSumPath);  // ACAD

    return true;
}

AcDbObjectId KTArxUtility::PostToModelSpace(AcDbEntity* pEnt, AcDbDatabase* pDb /*= acdbCurDwg()*/) {
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
    if (pBlkTblRec.openStatus() != Acad::eOk) return AcDbObjectId::kNull;
    AcDbObjectId entId = AcDbObjectId::kNull;
    if (Acad::eOk != pBlkTblRec->appendAcDbEntity(entId, pEnt)) {
        delete pEnt;
        pEnt = nullptr;
        return AcDbObjectId::kNull;
    }
    pEnt->close();
    return entId;
#endif
}

AcGePoint3dArray KTArxUtility::SortPtArr(const AcGePoint3dArray& PointArr, bool bVertical) {
    AcGePoint3dArray tempPointArr = PointArr;
    if (bVertical)  // 是垂直的
        std::sort(tempPointArr.asArrayPtr(), tempPointArr.asArrayPtr() + tempPointArr.length(), [](const AcGePoint3d& a, const AcGePoint3d& b) { return a.y < b.y; });
    else
        std::sort(tempPointArr.asArrayPtr(), tempPointArr.asArrayPtr() + tempPointArr.length(), [](const AcGePoint3d& a, const AcGePoint3d& b) { return a.x < b.x; });

    return tempPointArr;
}

void KTArxUtility::ShowProgressBar(const CString& strName, bool bStart) {
    if (bStart)
        acedSetStatusBarProgressMeter(strName, 0, 100);  // 显示进度条
    else
        acedRestoreStatusBar();  // 关闭进度条
}

void KTArxUtility::SetProgressBar(int nCurIndex, int nSum) {
    int nNum = (nCurIndex + 1) * 100 / nSum;
    acedSetStatusBarProgressMeterPos(nNum);  // 设置进度条的进度
}
}  // namespace KTArxTool
