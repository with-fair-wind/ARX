#include <KTArxTool/KTArxUtility.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <memory>

namespace KTArxTool {
CString KTArxUtility::getCurZrxFilePath() {
    std::array<TCHAR, _MAX_PATH> pathBuffer{};
    ::GetModuleFileName(_hdllInstance, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
    return pathBuffer.data();
}

CString KTArxUtility::getCurZrxFolderPath() {
    std::array<TCHAR, _MAX_PATH> pathBuffer{};
    ::GetModuleFileName(_hdllInstance, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
    CString strZrxPath = pathBuffer.data();
    int nIndex = strZrxPath.ReverseFind(_T('\\'));
    CString strFloder = strZrxPath.Left(nIndex + 1);
    return strFloder;
}

AcArray<CString> KTArxUtility::getAllDwgPath(LPCTSTR szFolderPath) {
    AcArray<CString> arrDwgPath;
    CFileFind finder;
    CString path{szFolderPath};
    if (path.GetLength() > 0 && path.Right(1) != _T("\\")) {
        path += _T("\\");
    }
    path += _T("*.dwg");

    bool isWorking = finder.FindFile(path) != FALSE;
    while (isWorking) {
        isWorking = finder.FindNextFileW() != FALSE;
        if (finder.IsDirectory() != FALSE || finder.IsDots() != FALSE) {
            continue;
        }
        CString strDwgPath = finder.GetFilePath();
        arrDwgPath.append(strDwgPath);
    }
    return arrDwgPath;
}

AcArray<CString> KTArxUtility::getAllDwgName(LPCTSTR szFolderPath) {
    AcArray<CString> arrDwgPath;
    CFileFind finder;
    CString path{szFolderPath};
    if (path.GetLength() > 0 && path.Right(1) != _T("\\")) {
        path += _T("\\");
    }
    path += _T("*.dwg");

    bool isWorking = finder.FindFile(path) != FALSE;
    while (isWorking) {
        isWorking = finder.FindNextFileW() != FALSE;
        if (finder.IsDirectory() != FALSE || finder.IsDots() != FALSE) {
            continue;
        }
        CString strDwgName = finder.GetFileTitle();
        arrDwgPath.append(strDwgName);
    }
    return arrDwgPath;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool KTArxUtility::loadCadMenu(const CString& strMenuFileName, const CString& strMenuGroupName) {
    CString strFilePath = strMenuFileName;
    CFileStatus fileStatus;
    if (CFile::GetStatus(strFilePath, fileStatus) == FALSE) {
        return false;
    }
    if (strMenuGroupName.GetLength() < 1 || strFilePath.GetLength() < 1) {
        return false;
    }
    CString strCmdLoad;
    strFilePath.Replace(_T("\\"), _T("\\\\"));
    if (!strMenuGroupName.IsEmpty() && strFilePath.GetLength() > 0) {
        strCmdLoad = _T("(if (null (menugroup \"") + strMenuGroupName + _T("\")) (command \"menuload\" \"") + strFilePath + _T("\"))");
    } else {
        strCmdLoad = _T("(command \"menuload\" \"") + strFilePath + _T("\")");
    }

    LPTSTR cmdBuffer = strCmdLoad.GetBuffer();
    const int queueStatus = ads_queueexpr(cmdBuffer);  // 此函数，相当于程序结束后，给CAD发送一个加载菜单的命令
    strCmdLoad.ReleaseBuffer();
    (void)queueStatus;
    return true;
}

bool KTArxUtility::unloadCadMenu(const CString& strMenuGroupName) {
    CString strCmdLoad;
    strCmdLoad = _T("(command \"menuunload\" \"") + strMenuGroupName + _T("\")");

    LPTSTR cmdBuffer = strCmdLoad.GetBuffer();
    const int queueStatus = ads_queueexpr(cmdBuffer);
    strCmdLoad.ReleaseBuffer();
    (void)queueStatus;
    return true;
}

bool KTArxUtility::loadZrxFile(LPCTSTR szFilePath) {
    bool bRet = false;
    CFileStatus fileStatus;
    if (CFile::GetStatus(szFilePath, fileStatus) == FALSE) {
        return false;
    }
    try {
        bRet = acrxLoadModule(&fileStatus.m_szFullName[0], true, false);
    } catch (...) {
        bRet = false;
    }
    return bRet;
}

bool KTArxUtility::addSearchPath(LPCTSTR szSearchDir) {
    CFileStatus fileStatus;
    if (CFile::GetStatus(szSearchDir, fileStatus) == FALSE) {
        return false;
    }
    const bool isDirectory = (static_cast<unsigned int>(fileStatus.m_attribute) & static_cast<unsigned int>(CFile::directory)) != 0U;
    if (!isDirectory) {
        return false;
    }

    CString strSumPath;
    constexpr std::size_t kEnvValueCapacity = 1024U;
    std::array<TCHAR, kEnvValueCapacity> envValue{};
    acedGetEnv(_T("ZWCAD"), envValue.data(), envValue.size());  // ACAD
    strSumPath = envValue.data();
    if (strSumPath.GetLength() < 1 || strSumPath.GetAt(strSumPath.GetLength() - 1) != _T(';')) {
        strSumPath += _T(';');
    }

    strSumPath = strSumPath + szSearchDir + _T(';');
    acedSetEnv(_T("ZWCAD"), strSumPath);  // ACAD

    return true;
}

AcDbObjectId KTArxUtility::postToModelSpace(AcDbEntity* pEnt, AcDbDatabase* pDb /*= acdbCurDwg()*/) {
    assert(pEnt != nullptr);
    assert(pDb != nullptr);

    std::unique_ptr<AcDbEntity> entOwner{pEnt};

    AcDbBlockTableRecordPointer pBlkTblRec(pDb->currentSpaceId(), AcDb::kForWrite);
    if (pBlkTblRec.openStatus() != Acad::eOk) {
        return AcDbObjectId::kNull;
    }
    AcDbObjectId entId = AcDbObjectId::kNull;
    if (Acad::eOk != pBlkTblRec->appendAcDbEntity(entId, entOwner.get())) {
        return AcDbObjectId::kNull;
    }
    (void)entOwner->close();
    AcDbEntity* const releasedEnt = entOwner.release();  // ownership transferred to the database
    (void)releasedEnt;
    return entId;
}

AcGePoint3dArray KTArxUtility::sortPtArr(const AcGePoint3dArray& PointArr, bool bVertical) {
    AcGePoint3dArray tempPointArr = PointArr;
    if (tempPointArr.length() <= 1) {
        return tempPointArr;
    }

    AcGePoint3d* beginPtr = tempPointArr.asArrayPtr();
    AcGePoint3d* endPtr = beginPtr;
    std::advance(endPtr, tempPointArr.length());

    if (bVertical) {  // 是垂直的
        std::sort(beginPtr, endPtr, [](const AcGePoint3d& lhs, const AcGePoint3d& rhs) { return lhs.y < rhs.y; });
    } else {
        std::sort(beginPtr, endPtr, [](const AcGePoint3d& lhs, const AcGePoint3d& rhs) { return lhs.x < rhs.x; });
    }

    return tempPointArr;
}

void KTArxUtility::showProgressBar(const CString& strName, bool bStart) {
    constexpr int kProgressMin = 0;
    constexpr int kProgressMax = 100;

    if (bStart) {
        acedSetStatusBarProgressMeter(strName, kProgressMin, kProgressMax);  // 显示进度条
    } else {
        acedRestoreStatusBar();  // 关闭进度条
    }
}

void KTArxUtility::setProgressBar(int nCurIndex, int nSum) {
    constexpr int kProgressMax = 100;
    if (nSum <= 0) {
        acedSetStatusBarProgressMeterPos(0);
        return;
    }

    int nNum = (nCurIndex + 1) * kProgressMax / nSum;
    acedSetStatusBarProgressMeterPos(nNum);  // 设置进度条的进度
}
}  // namespace KTArxTool
