#include <Entity/testEntity.h>
#include <Test/command.h>

#include <sstream>

void registerEntity() {
    Test_Entity::TestEntity::rxInit();
    Test_Entity::TestEntity2::rxInit();
}

void deleteEntity() {
    deleteAcRxClass(Test_Entity::TestEntity::desc());
    deleteAcRxClass(Test_Entity::TestEntity2::desc());
}

void initApp() {
    acedRegCmds->addCommand(_T("TestCMD"), _T("test0"), _T("test0"), ACRX_CMD_TRANSPARENT, Test_Command::test1);
    acedRegCmds->addCommand(_T("TestCMD"), _T("test1"), _T("test1"), ACRX_CMD_TRANSPARENT, Test_Command::test2);
    acedRegCmds->addCommand(_T("TestCMD"), _T("test3"), _T("test3"), ACRX_CMD_TRANSPARENT, Test_Command::test3);
    acedRegCmds->addCommand(_T("TestCMD"), _T("test4"), _T("test4"), ACRX_CMD_TRANSPARENT, Test_Command::test4);
    acedRegCmds->addCommand(_T("TestCMD"), _T("test5"), _T("test5"), ACRX_CMD_TRANSPARENT, Test_Command::test5);

    registerEntity();
    acrxBuildClassHierarchy();
}

void unloadApp() {
    deleteEntity();
    acedRegCmds->removeGroup(_T("TestCMD"));
}

extern "C" AcRx::AppRetCode zcrxEntryPoint(AcRx::AppMsgCode msg, void* pkt) {
    switch (msg) {
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
