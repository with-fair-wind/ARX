#include <Command/command.h>
#include <Services/app_editor_reactor.h>
#include <Services/app_event_service.h>
#include <Services/zcBm_lisp_bridge.h>

// CmfcApp
class CmfcApp : public CWinApp {
   public:
    CmfcApp() = default;

    // 重写
    BOOL InitInstance() override;
    int ExitInstance() override;

    DECLARE_MESSAGE_MAP()
};

// 唯一的 CmfcApp 对象
namespace {
CmfcApp theApp;                          // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp)
AppEditorReactor* g_pReactor = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}  // namespace

// CmfcApp 初始化
BOOL CmfcApp::InitInstance() {
    CWinApp::InitInstance();

    // 初始化 OLE 库
    if (AfxOleInit() == FALSE) {
        AfxMessageBox(_T("OLE initialization failed."));
        return FALSE;
    }

    return TRUE;
}

int CmfcApp::ExitInstance() { return CWinApp::ExitInstance(); }

BEGIN_MESSAGE_MAP(CmfcApp, CWinApp)
END_MESSAGE_MAP()

// DLL 主入口点
extern "C" AcRx::AppRetCode zcrxEntryPoint(AcRx::AppMsgCode msg, void* pkt) {
    switch (msg) {
        case AcRx::kInitAppMsg:
            acrxDynamicLinker->unlockApplication(pkt);
            acrxRegisterAppMDIAware(pkt);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCTEST"), _T("MFCTEST"), ACRX_CMD_MODAL, MfcTestCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCARXUI"), _T("MFCARXUI"), ACRX_CMD_MODAL, MfcArxUiDialogCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCENT"), _T("MFCENT"), ACRX_CMD_MODAL, MfcCreateEntity);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCLOADDOC"), _T("MFCLOADDOC"), ACRX_CMD_MODAL, MfcLoadToDocCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCBROWSER"), _T("MFCBROWSER"), ACRX_CMD_MODAL, MfcComponentBrowserCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCSAVEAS"), _T("MFCSAVEAS"), ACRX_CMD_MODAL, MfcSaveAsDialogCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCTESTRENAME"), _T("MFCTESTRENAME"), ACRX_CMD_MODAL, MfcTestRenameDialogCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCTESTOVERWRITE"), _T("MFCTESTOVERWRITE"), ACRX_CMD_MODAL, MfcTestOverwriteDialogCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCPROPPERM"), _T("MFCPROPPERM"), ACRX_CMD_MODAL, MfcPropertyPermissionCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCNEWFAMILY"), _T("MFCNEWFAMILY"), ACRX_CMD_MODAL, MfcNewFamilyCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCNEWFAMILYDEMO"), _T("MFCNEWFAMILYDEMO"), ACRX_CMD_MODAL, MfcNewFamilyTempDemoCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCLISPTEST0"), _T("MFCLISPTEST0"), ACRX_CMD_MODAL, MfcLispInvokeNoArgsCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCLISPTEST1"), _T("MFCLISPTEST1"), ACRX_CMD_MODAL, MfcLispInvokeIntRealCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCLISPTEST2"), _T("MFCLISPTEST2"), ACRX_CMD_MODAL, MfcLispInvokePointStringCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCLISPCATTEST"), _T("MFCLISPCATTEST"), ACRX_CMD_MODAL, MfcLispValueCategoryTestCommand);
            acedRegCmds->addCommand(_T("MFCGROUP"), _T("MFCLISPDYNTEST"), _T("MFCLISPDYNTEST"), ACRX_CMD_MODAL, MfcLispDynamicApiTestCommand);
            // 注册 EditorReactor, 用于 commandEnded 时 flush 事件
            g_pReactor = new AppEditorReactor();
            acedEditor->addReactor(g_pReactor);

            acutPrintf(_T("\nMFC ARX 应用程序已加载。"));
            acutPrintf(_T("\n可用命令: MFCTEST, MFCARXUI, MFCENT, MFCLOADDOC, MFCBROWSER, MFCSAVEAS, MFCTESTRENAME, MFCTESTOVERWRITE, MFCPROPPERM, MFCNEWFAMILY, MFCNEWFAMILYDEMO, MFCLISPTEST0/1/2, MFCLISPCATTEST, MFCLISPDYNTEST"));
            break;

        case AcRx::kLoadDwgMsg:
            ZcBmRegisterLispCommands();
            break;

        case AcRx::kInvkSubrMsg:
            ZcBmDispatchLispCommand();
            break;

        case AcRx::kUnloadAppMsg:
            // 注销 EditorReactor
            if (g_pReactor != nullptr) {
                acedEditor->removeReactor(g_pReactor);
                delete g_pReactor;
                g_pReactor = nullptr;
            }
            AppEventService::instance().reset();
            ZcBmUnregisterLispCommands();

            acedRegCmds->removeGroup(_T("MFCGROUP"));
            acutPrintf(_T("\nMFC ARX 应用程序已卸载。"));
            break;
        default:
            break;
    }

    return AcRx::kRetOK;
}

#pragma comment(linker, "/export:zcrxEntryPoint,PRIVATE")
#pragma comment(linker, "/export:zcrxGetApiVersion,PRIVATE")
