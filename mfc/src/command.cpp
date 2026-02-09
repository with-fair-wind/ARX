#include <Command/command.h>
#include <Dialog/arxui_dialog.h>
#include <Dialog/load_to_doc_dialog.h>
#include <Dialog/test_dialog.h>

// 示例命令：显示MFC对话框
void MfcTestCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行MFC测试命令..."));

    // 获取CAD主窗口句柄
    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());

    // 创建并显示对话框
    CTestDialog dlg(pMainWnd);
    if (dlg.DoModal() == IDOK) {
        acutPrintf(_T("\n对话框已关闭。"));
    }
}

// 示例命令：显示ZWCAD ARX UI对话框
void MfcArxUiDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行ARX UI对话框命令..."));

    // 获取CAD主窗口句柄
    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());

    // 创建并显示对话框
    CArxUiDemoDialog dlg(pMainWnd);
    dlg.DoModal();
}

// 示例命令：创建简单实体
void MfcCreateEntity() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行创建实体命令..."));

    // 获取两个点
    AcGePoint3d pt1, pt2;
    if (acedGetPoint(nullptr, _T("\n选择第一个点: "), asDblArray(pt1)) != RTNORM) {
        acutPrintf(_T("\n命令已取消。"));
        return;
    }

    if (acedGetPoint(asDblArray(pt1), _T("\n选择第二个点: "), asDblArray(pt2)) != RTNORM) {
        acutPrintf(_T("\n命令已取消。"));
        return;
    }

    // 创建一条线
    AcDbLine* pLine = new AcDbLine(pt1, pt2);
    if (pLine == nullptr) {
        acutPrintf(_T("\n创建直线失败。"));
        return;
    }

    // 将线添加到数据库
    AcDbBlockTable* pBlockTable = nullptr;
    AcDbBlockTableRecord* pBlockTableRecord = nullptr;
    acdbHostApplicationServices()->workingDatabase()->getBlockTable(pBlockTable, AcDb::kForRead);
    pBlockTable->getAt(ACDB_MODEL_SPACE, pBlockTableRecord, AcDb::kForWrite);
    pBlockTable->close();

    AcDbObjectId lineId;
    pBlockTableRecord->appendAcDbEntity(lineId, pLine);
    pBlockTableRecord->close();
    pLine->close();

    acutPrintf(_T("\n直线创建成功！"));

    // 显示MFC消息框确认
    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    AfxMessageBox(_T("直线已成功创建！"), MB_OK | MB_ICONINFORMATION);
}

// 载入至文档命令
void MfcLoadToDocCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行载入至文档命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    CLoadToDocDialog dlg(pMainWnd);
    dlg.DoModal();
}
