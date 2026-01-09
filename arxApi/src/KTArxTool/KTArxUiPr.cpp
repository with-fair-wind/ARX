
namespace KTArxTool {
bool KTArxUiPr::SelEnt(LPCTSTR szPrompt, AcRxClass* classType, AcDbObjectId& id, AcGePoint3d& pt) {
    CString strPrompt;
    strPrompt.Format(_T("\n%s："), szPrompt);
    while (true) {
        ads_name ent;
        ads_point adsPt;
        if (int ret = acedEntSel(strPrompt, ent, adsPt); ret == RTCAN || ret == RTNONE || ret == RTERROR)
            return false;
        else if (RTNORM != ret)
            continue;

        AcDbObjectId tmpId;
        acdbGetObjectId(tmpId, ent);
        AcDbEntity* pEnt = nullptr;
        if (Acad::ErrorStatus es = acdbOpenAcDbEntity(pEnt, tmpId, AcDb::kForRead); Acad::eOk != es) return false;
        AcRxClass* rxClass = static_cast<AcRxClass*>(classType);
        if (pEnt->isKindOf(rxClass))  // 如果“this”对象是由aClass表示的类的成员，或者是从aClass派生的类的成员，则返回true。
        {
            pEnt->close();
            pt = asPnt3d(adsPt);
            id = tmpId;
            return true;
        }
        pEnt->close();
    }
    return false;
}
}  // namespace KTArxTool
