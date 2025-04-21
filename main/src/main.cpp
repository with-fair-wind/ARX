#include <sstream>
#include <Test/command.h>

void initApp()
{
    acedRegCmds->addCommand(_T("TestCMD"), _T("test0"), _T("test0"), ACRX_CMD_TRANSPARENT, Test_Command::test1);
    acedRegCmds->addCommand(_T("TestCMD"), _T("test1"), _T("test1"), ACRX_CMD_TRANSPARENT, Test_Command::test2);
    acrxBuildClassHierarchy();
}

void unloadApp()
{
    acedRegCmds->removeGroup(_T("TestCMD"));
}

extern "C" AcRx::AppRetCode
zcrxEntryPoint(AcRx::AppMsgCode msg, void *pkt)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:
        acrxDynamicLinker->unlockApplication(pkt);
        acrxRegisterAppMDIAware(pkt);
        initApp();
        break;
    case AcRx::kUnloadAppMsg:
        unloadApp();
        break;
    default:
        break;
    }

    return AcRx::kRetOK;
}

#pragma comment(linker, "/export:zcrxEntryPoint,PRIVATE")
#pragma comment(linker, "/export:zcrxGetApiVersion,PRIVATE")
