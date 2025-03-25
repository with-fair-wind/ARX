namespace KTArxTool
{
    bool KTArxEntity::ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase)
    {
        AcGeMatrix3d mtx;
        mtx.setToScaling(dScale, ptBase);
        return pEnt->transformBy(mtx) == Acad::eOk;
    }

    bool KTArxEntity::ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase)
    {
        AcDbEntityPointer pEnt(idEnt, AcDb::kForWrite);
        if (pEnt.openStatus() != Acad::eOk)
            return false;
        return ModifyEntByScale(pEnt, dScale, ptBase);
    }

    bool KTArxEntity::ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale,
                                       const AcGePoint3d &ptBase)
    {
        bool flag = true;
        for (auto &cur : arrpEnt)
            if (flag = ModifyEntByScale(cur, dScale, ptBase); !flag)
                break;
        return flag;
    }

    AcArray<AcDbEntity *> KTArxEntity::ExplodeEnt(AcDbEntity *pEnt)
    {
        AcArray<AcDbEntity *> arrExpEnt;
        AcDbVoidPtrArray exArr;
        if (pEnt->explode(exArr) == Acad::eOk)
        {
            for (int i = 0; i < exArr.length(); i++)
                arrExpEnt.append(static_cast<AcDbEntity *>(exArr.at(i)));
        }
        return arrExpEnt;
    }

    AcArray<AcDbEntity *> KTArxEntity::ExplodeEnt(const AcDbObjectId &idEnt)
    {
        AcArray<AcDbEntity *> arrExpEnt;
        AcDbEntityPointer pEnt(idEnt, AcDb::kForRead);
        if (Acad::eOk != pEnt.openStatus())
            return arrExpEnt;
        return ExplodeEnt(pEnt);
    }

    bool KTArxEntity::MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        assert(pEnt);
        AcGeMatrix3d xform;
        AcGeVector3d vec(ptDest - ptBase);
        xform.setToTranslation(vec);
        return pEnt->transformBy(xform) == Acad::eOk;
    }

    bool KTArxEntity::MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        bool flag = true;
        for (auto &pEnt : arrEnt)
        {
            if (!pEnt)
                continue;
            if (flag = MoveEnt(pEnt, ptBase, ptDest); !flag)
                break;
        }
        return flag;
    }

    bool KTArxEntity::MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        AcDbEntityPointer pEnt(idEnt, AcDb::kForWrite);
        if (Acad::eOk != pEnt.openStatus())
            return false;
        return MoveEnt(pEnt, ptBase, ptDest);
    }

    bool KTArxEntity::MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest)
    {
        bool flag = true;
        for (int i = 0; i < arrid.length(); i++)
        {
            AcDbEntityPointer pEnt(arrid[i], AcDb::kForWrite);
            if (Acad::eOk != pEnt.openStatus())
                continue;
            if (flag = MoveEnt(pEnt, ptBase, ptDest); !flag)
                break;
        }
        return flag;
    }

    bool KTArxEntity::RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation)
    {
        assert(pEnt);
        AcGeMatrix3d xform;
        xform.setToRotation(rotation, AcGeVector3d::kZAxis, ptBase); // 参数二是输入旋转轴向量
        return pEnt->transformBy(xform) == Acad::eOk;
    }

    bool KTArxEntity::RotateEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, double rotation)
    {
        bool flag = true;
        for (auto &pEnt : arrEnt)
        {
            if (!pEnt)
                continue;
            if (flag = RotateEnt(pEnt, ptBase, rotation); !flag)
                break;
        }
        return flag;
    }

    bool KTArxEntity::DeleteEnt(const AcDbObjectId &idEnt)
    {
        AcDbEntityPointer pEnt(idEnt, AcDb::kForWrite);
        if (Acad::eOk != pEnt.openStatus())
            return false;
        return pEnt->erase() == Acad::eOk;
    }

    void KTArxEntity::DeleteEnt(AcArray<AcDbEntity *> &arrpEnt)
    {
        for (int i = 0; i < arrpEnt.length(); i++)
        {
            DEL(arrpEnt[i]);
        }
    }

    AcArray<AcDbEntity *> KTArxEntity::CopyEnt(const AcDbObjectIdArray &arrid)
    {
        AcArray<AcDbEntity *> arrEnt;
        for (int i = (arrid.length() - 1); i >= 0; i--)
        {
            AcDbEntityPointer pEnt(arrid[i], AcDb::kForRead);
            if (Acad::eOk != pEnt.openStatus())
                continue;
            AcDbEntity *pCopy = AcDbEntity::cast(pEnt->clone());
            if (pCopy)
                arrEnt.append(pCopy);
        }
        return arrEnt;
    }

    AcArray<AcDbEntity *> KTArxEntity::CopyEnt(const AcArray<AcDbEntity *> &arrp)
    {
        return AcArray<AcDbEntity *>();
    }

    AcDbEntity *KTArxEntity::CopyEnt(const AcDbObjectId &idEnt)
    {
        return nullptr;
    }

    AcDbEntity *KTArxEntity::MirrorEnt(AcDbEntity *pEnt, const AcGePoint3d &pt1, const AcGePoint3d &pt2)
    {
        return nullptr;
    }

    AcArray<AcDbEntity *> KTArxEntity::MirrorEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &pt1, const AcGePoint3d &pt2)
    {
        return AcArray<AcDbEntity *>();
    }

    AcArray<AcDbEntity *> KTArxEntity::MirrorEnt(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &pt1, const AcGePoint3d &pt2)
    {
        return AcArray<AcDbEntity *>();
    }

    AcArray<AcDbEntity *> KTArxEntity::AnnularMatrix(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, int nCurNum, int nSum)
    {
        return AcArray<AcDbEntity *>();
    }

    AcArray<AcDbEntity *> KTArxEntity::AnnularMatrix(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &ptBase, int nYxcs, int nSum)
    {
        return AcArray<AcDbEntity *>();
    }

    bool KTArxEntity::IsClosedPline(AcDbPolyline *pPline)
    {
        if (pPline->isClosed())
            return true;
        AcGePoint3d ptStart, ptEnd;
        pPline->getStartPoint(ptStart);
        pPline->getEndPoint(ptEnd);
        return ptStart.isEqualTo(ptEnd);
    }
} // namespace KTArxTool