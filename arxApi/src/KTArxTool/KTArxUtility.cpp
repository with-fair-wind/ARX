namespace KTArxTool
{
    AcDbObjectId KTArxUtility::postToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb /*= acdbCurDwg()*/)
    {
        assert(pEnt);
#if 0
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
            acutPrintf(_T("添加失败"));
            delete pEnt; // 添加失败时，要delete;
            pEnt = nullptr;

            return AcDbObjectId::kNull;
        }

        pBlkTblRcd->close();
        pEnt->close();

        return entId;
#else
        AcDbBlockTableRecordPointer pBlkTblRec(pDb->currentSpaceId(), AcDb::kForWrite);
        if (pBlkTblRec.openStatus() != Acad::eOk)
            return AcDbObjectId::kNull;
        AcDbObjectId entId = AcDbObjectId::kNull;
        if (Acad::eOk != pBlkTblRec->appendAcDbEntity(entId, pEnt))
        {
            delete pEnt;
            pEnt = nullptr;
            return AcDbObjectId::kNull;
        }
        pEnt->close();
        return entId;
#endif
    }
} // namespace KK
