#include <KTArxTool/KTArxUiPr.h>

#include <array>
#include <type_traits>

namespace KTArxTool {
bool KTArxUiPr::selEnt(LPCTSTR prompt, AcRxClass* classType, AcDbObjectId& objectId, AcGePoint3d& pickedPoint) {
    CString strPrompt{_T("\n")};
    strPrompt += prompt;
    strPrompt += _T(": ");

    while (true) {
        std::array<std::remove_extent_t<ads_name>, std::extent_v<ads_name>> ent{};
        std::array<std::remove_extent_t<ads_point>, std::extent_v<ads_point>> adsPt{};

        const int ret = acedEntSel(strPrompt.GetString(), ent.data(), adsPt.data());
        if (ret == RTCAN || ret == RTNONE || ret == RTERROR) {
            return false;
        }
        if (ret != RTNORM) {
            continue;
        }

        AcDbObjectId tmpId;
        acdbGetObjectId(tmpId, ent.data());

        AcDbEntity* pEnt = nullptr;
        const Acad::ErrorStatus openStatus = acdbOpenAcDbEntity(pEnt, tmpId, AcDb::kForRead);
        if (openStatus != Acad::eOk) {
            return false;
        }

        if (pEnt->isKindOf(classType))  // 如果“this”对象是由aClass表示的类的成员，或者是从aClass派生的类的成员，则返回true。
        {
            pEnt->close();
            pickedPoint = AcGePoint3d{adsPt[0], adsPt[1], adsPt[2]};
            objectId = tmpId;
            return true;
        }
        pEnt->close();
    }
    return false;
}
}  // namespace KTArxTool
