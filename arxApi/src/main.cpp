#define szRDS _RXST("")

class CObjectArxApp : public AcRxArxApp
{
public:
    CObjectArxApp() : AcRxArxApp() {}

    virtual AcRx::AppRetCode On_kInitAppMsg(void *pkt) override
    {
        // TODO: �������������������������

        // You *must* call On_kInitAppMsg here
        AcRx::AppRetCode retCode = AcRxArxApp::On_kInitAppMsg(pkt);

        // TODO: ��ʼ��

        return (retCode);
    }

    virtual AcRx::AppRetCode On_kUnloadAppMsg(void *pkt) override
    {
        // TODO: �˳�

        // You *must* call On_kUnloadAppMsg here
        AcRx::AppRetCode retCode = AcRxArxApp::On_kUnloadAppMsg(pkt);

        // TODO: ж������

        return (retCode);
    }
    virtual void RegisterServerComponents() override {}
};

IMPLEMENT_ARX_ENTRYPOINT(CObjectArxApp)