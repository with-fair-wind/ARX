#define szRDS _RXST("")

class CObjectArxApp : public AcRxArxApp
{
public:
    CObjectArxApp() : AcRxArxApp() {}

    virtual AcRx::AppRetCode On_kInitAppMsg(void *pkt) override
    {
        // TODO: 如果有其他依赖项，请在这里加载

        // You *must* call On_kInitAppMsg here
        AcRx::AppRetCode retCode = AcRxArxApp::On_kInitAppMsg(pkt);

        // TODO: 初始化

        return (retCode);
    }

    virtual AcRx::AppRetCode On_kUnloadAppMsg(void *pkt) override
    {
        // TODO: 退出

        // You *must* call On_kUnloadAppMsg here
        AcRx::AppRetCode retCode = AcRxArxApp::On_kUnloadAppMsg(pkt);

        // TODO: 卸载依赖

        return (retCode);
    }
    virtual void RegisterServerComponents() override {}
};

IMPLEMENT_ARX_ENTRYPOINT(CObjectArxApp)