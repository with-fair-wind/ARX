// Ensure MFC extension DLL symbols are defined in a single TU
#include <afxdllx.h>

#define szRDS _RXST("")

class CObjectArxApp : public AcRxArxApp {
   public:
    CObjectArxApp() : AcRxArxApp() {}
    CObjectArxApp(const CObjectArxApp&) = delete;
    CObjectArxApp& operator=(const CObjectArxApp&) = delete;
    CObjectArxApp(CObjectArxApp&&) = delete;
    CObjectArxApp& operator=(CObjectArxApp&&) = delete;

    virtual ~CObjectArxApp() = default;

    AcRx::AppRetCode On_kInitAppMsg(void* pkt) override {
        // TODO: 如果有其他依赖项，请在这里加载

        // You *must* call On_kInitAppMsg here
        AcRx::AppRetCode retCode = AcRxArxApp::On_kInitAppMsg(pkt);

        // TODO: 初始化

        return (retCode);
    }

    AcRx::AppRetCode On_kUnloadAppMsg(void* pkt) override {
        // TODO: 退出

        // You *must* call On_kUnloadAppMsg here
        AcRx::AppRetCode retCode = AcRxArxApp::On_kUnloadAppMsg(pkt);

        // TODO: 卸载依赖

        return (retCode);
    }
    void RegisterServerComponents() override {}
};

IMPLEMENT_ARX_ENTRYPOINT(CObjectArxApp)