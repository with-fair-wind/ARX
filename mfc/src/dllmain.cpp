#include <Command/command.h>

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
CmfcApp theApp;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp)
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
            acutPrintf(_T("\nMFC ARX 应用程序已加载。"));
            acutPrintf(_T("\n可用命令: MFCTEST, MFCARXUI, MFCENT"));
            break;

        case AcRx::kUnloadAppMsg:
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
