namespace KK
{
    AcDbObjectId KKArxUtility::postToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb /*= acdbCurDwg()*/)
    {
        assert(pEnt);
        AcDbBlockTable *pBlkTbl = nullptr;
        pDb->getBlockTable(pBlkTbl, AcDb::kForRead);
        AcDbBlockTableRecord *pBlkTblRcd = nullptr;
        pBlkTbl->getAt(ACDB_MODEL_SPACE, pBlkTblRcd, AcDb::kForWrite);
        pBlkTbl->close();

        AcDbObjectId entId = AcDbObjectId::kNull;
        Acad::ErrorStatus es = pBlkTblRcd->appendAcDbEntity(entId, pEnt);
        if (es != Acad::eOk)
        {
            pBlkTblRcd->close();
            acutPrintf(_T("���ʧ��"));
            delete pEnt; // ���ʧ��ʱ��Ҫdelete;
            pEnt = nullptr;

            return AcDbObjectId::kNull;
        }

        pBlkTblRcd->close();
        pEnt->close();

        return entId;
    }

} // namespace KK
