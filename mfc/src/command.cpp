#include <Command/command.h>
#include <Dialog/arxui_dialog.h>
#include <Dialog/component_browser_dialog.h>
#include <Dialog/load_to_doc_dialog.h>
#include <Dialog/new_family_dialog.h>
#include <Dialog/overwrite_confirm_dialog.h>
#include <Dialog/property_permission_dialog.h>
#include <Dialog/rename_template_dialog.h>
#include <Dialog/test_dialog.h>
#include <Services/new_family_backend.h>
#include <acedads.h>
#include <aduiFileDialog.h>

#include "acdocman.h"

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
    ZcBmLoadToDocDialog dlg(pMainWnd);
    dlg.DoModal();
}

// 构件浏览器命令
void MfcComponentBrowserCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmComponentBrowserDialog dlg(pMainWnd);
    dlg.DoModal();
}

// 示例命令：触发CAD另存为对话框
void MfcSaveAsDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行另存为测试命令（API模式）..."));

    AcApDocument* currentDoc = acDocManager->curDocument();
    if (currentDoc == nullptr || currentDoc->database() == nullptr) {
        acutPrintf(_T("\n无法获取当前文档。"));
        return;
    }

    CString defaultName = currentDoc->fileName();
    if (defaultName.IsEmpty()) {
        defaultName = _T("未命名.dwg");
    }

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    CAdUiFileDialog dlg(FALSE, _T("dwg"), defaultName, OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, _T("Drawing (*.dwg)|*.dwg||"), pMainWnd);
    if (dlg.DoModal() != IDOK) {
        acutPrintf(_T("\n已取消另存为。"));
        return;
    }

    CString targetPath = dlg.GetPathName();
    if (targetPath.IsEmpty()) {
        acutPrintf(_T("\n未选择有效保存路径。"));
        return;
    }

    CString lowerPath = targetPath;
    lowerPath.MakeLower();
    if (lowerPath.GetLength() < 4 || lowerPath.Right(4) != _T(".dwg")) {
        targetPath += _T(".dwg");
    }

    Acad::ErrorStatus status = currentDoc->database()->saveAs(targetPath.GetString(), true, AcDb::kDHL_CURRENT);
    if (status != Acad::eOk) {
        acutPrintf(_T("\n另存为失败，错误码: %d"), static_cast<int>(status));
        return;
    }

    acutPrintf(_T("\n另存为成功: %s"), targetPath.GetString());
}

void MfcTestRenameDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行重命名对话框测试命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmRenameTemplateDialog dlg(_T("示例构件A"), pMainWnd);
    const INT_PTR result = dlg.DoModal();

    if (result == IDOK) {
        acutPrintf(_T("\n[RenameDialog] 点击确定，新名称: %s"), dlg.getNewName().GetString());
        return;
    }

    if (dlg.isCancelLoad()) {
        acutPrintf(_T("\n[RenameDialog] 点击取消载入。"));
        return;
    }

    acutPrintf(_T("\n[RenameDialog] 对话框关闭，返回码: %Id"), result);
}

void MfcTestOverwriteDialogCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行覆盖确认对话框测试命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmOverwriteConfirmDialog dlg(pMainWnd);
    const INT_PTR result = dlg.DoModal();
    const OverwriteChoice choice = dlg.choice();

    if (result == IDOK && choice == OverwriteChoice::kUpdateAll) {
        acutPrintf(_T("\n[OverwriteDialog] 选择: 全部更新。"));
        return;
    }

    if (result == IDOK && choice == OverwriteChoice::kSkipParams) {
        acutPrintf(_T("\n[OverwriteDialog] 选择: 不覆盖参数值。"));
        return;
    }

    acutPrintf(_T("\n[OverwriteDialog] 选择: 取消载入。"));
}

void MfcPropertyPermissionCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行属性权限设置命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmPropertyPermissionDialog dlg(pMainWnd);
    dlg.DoModal();
}

struct AppContextInvoker {
    template <typename Fn, typename... Args>
    explicit AppContextInvoker(Fn&& func, Args&&... args) {
        using Tuple = std::tuple<std::decay_t<Fn>, std::decay_t<Args>...>;
        auto Decay_copied = std::make_unique<Tuple>(std::forward<Fn>(func), std::forward<Args>(args)...);
        auto Invoker_proc = start<Tuple>(std::make_index_sequence<1 + sizeof...(Args)>{});
        acDocManager->executeInApplicationContext(Invoker_proc, Decay_copied.release());
    }

    template <typename Tuple, std::size_t... Indices>
    static constexpr auto start(std::index_sequence<Indices...> /*indices*/) noexcept {
        return &Invoke<Tuple, Indices...>;  // 返回函数指针
    }

    template <class Tuple, std::size_t... Indices>
    static void Invoke(void* RawVals) noexcept {
        const std::unique_ptr<Tuple> FnVals(static_cast<Tuple*>(RawVals));
        Tuple& Tup = *FnVals.get();
        std::invoke(std::move(std::get<Indices>(Tup))...);  // 真正的调用
    }
};

void MfcNewFamilyCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行新建族命令..."));

    CWnd* pMainWnd = CWnd::FromHandle(adsw_acadMainWnd());
    ZcBmNewFamilyDialog dlg(pMainWnd);
    if (dlg.DoModal() != IDOK) {
        acutPrintf(_T("\n已取消新建族。"));
        return;
    }

    const NewFamilyRequest request = dlg.request();
    AppContextInvoker call{[request]() {
        std::wstring error_message;
        if (!executeCreateNewFamily(request, &error_message)) {
            acutPrintf(_T("\n新建族失败。"));
            if (!error_message.empty()) {
                acutPrintf(_T("\n错误详情: %ls"), error_message.c_str());
            }
            return;
        }
        acutPrintf(_T("\n新建族流程执行成功。"));
        // 这里是CAD应用上下文中执行的代码，可以安全调用CAD API。
    }};
}

void MfcNewFamilyTempDemoCommand() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    acutPrintf(_T("\n执行新建族临时文档链路 Demo..."));

    CWnd* main_wnd = CWnd::FromHandle(adsw_acadMainWnd());
    CAdUiFileDialog dlg(TRUE, _T("ztf"), _T(""), OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                        _T("模板文件 (*.ztf)|*.ztf|Drawing 文件 (*.dwg)|*.dwg|所有文件 (*.*)|*.*||"), main_wnd);
    if (dlg.DoModal() != IDOK) {
        acutPrintf(_T("\n已取消 Demo。"));
        return;
    }

    const std::wstring template_path = dlg.GetPathName().GetString();
    AppContextInvoker call{[template_path]() {
        std::wstring error_message;
        if (!executeCreateNewFamilyTempDemo(template_path, &error_message)) {
            acutPrintf(_T("\n[Demo] 临时文档链路失败。"));
            if (!error_message.empty()) {
                acutPrintf(_T("\n[Demo] 错误详情: %ls"), error_message.c_str());
            }
            return;
        }
        acutPrintf(_T("\n[Demo] 临时文档链路执行成功。"));
    }};
}
