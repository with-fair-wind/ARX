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
        AcArray<AcDbEntity *> arrEnt;
        for (int i = (arrp.length() - 1); i >= 0; i--)
        {
            AcDbEntity *pCopy = AcDbEntity::cast(arrp[i]->clone());
            if (pCopy)
                arrEnt.append(pCopy);
        }
        return arrEnt;
    }

    AcDbEntity *KTArxEntity::CopyEnt(const AcDbObjectId &idEnt)
    {
        AcDbEntityPointer pEnt(idEnt, AcDb::kForRead);
        if (Acad::eOk != pEnt.openStatus())
            return nullptr;
        return AcDbEntity::cast(pEnt->clone());
    }

    AcDbEntity *KTArxEntity::MirrorEnt(AcDbEntity *pEnt, const AcGePoint3d &pt1, const AcGePoint3d &pt2)
    {
        AcGeMatrix3d mat;
        mat.setToMirroring(AcGeLine3d(pt1, pt2));
        AcDbEntity *pCopy = AcDbEntity::cast(pEnt->clone());
        pCopy->transformBy(mat);

        AcGeMatrix3d mat2;
        mat2.setToMirroring(AcGePlane::kXYPlane);
        pCopy->transformBy(mat2);
        return pCopy;
    }

    AcArray<AcDbEntity *> KTArxEntity::MirrorEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &pt1, const AcGePoint3d &pt2)
    {
        AcArray<AcDbEntity *> arrpEnt;
        AcGeMatrix3d mat;
        mat.setToMirroring(AcGeLine3d(pt1, pt2));

        for (int i = 0; i < arrid.length(); i++)
        {
            AcDbEntityPointer pEnt(arrid[i], AcDb::kForRead);
            if (Acad::eOk != pEnt.openStatus())
                continue;
            AcDbEntity *pCopy = AcDbEntity::cast(pEnt->clone());
            pCopy->transformBy(mat);
            arrpEnt.append(pCopy);
        }

        return arrpEnt;
    }

    AcArray<AcDbEntity *> KTArxEntity::MirrorEnt(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &pt1, const AcGePoint3d &pt2)
    {
        AcArray<AcDbEntity *> arrpEnt;
        AcGeMatrix3d mat;
        mat.setToMirroring(AcGeLine3d(pt1, pt2));

        for (int i = 0; i < arrp.length(); i++)
        {
            AcDbEntity *pEnt = arrp[i];
            AcDbEntity *pCopy = AcDbEntity::cast(pEnt->clone());
            pCopy->transformBy(mat);
            arrpEnt.append(pCopy);
        }

        return arrpEnt;
    }

    AcArray<AcDbEntity *> KTArxEntity::AnnularMatrix(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, int nCurNum, int nSum)
    {
        AcArray<AcDbEntity *> arrpMatrix;

        if (nSum < 2 || arrid.isEmpty())
            return arrpMatrix;

        AcArray<double> arrAngle;
        double dAngle = 360 / nSum;
        for (int i = 1; i < nCurNum; i++)
            arrAngle.append(dAngle * i);

        for (int i = 0; i < arrAngle.length(); i++)
        {
            AcArray<AcDbEntity *> arrpCopy = CopyEnt(arrid);
            RotateEnt(arrpCopy, ptBase, m_pArxConvert->ToRadian(arrAngle[i]));
            arrpMatrix.append(arrpCopy);
        }

        return arrpMatrix;
    }

    AcArray<AcDbEntity *> KTArxEntity::AnnularMatrix(const AcArray<AcDbEntity *> &arrp, const AcGePoint3d &ptBase, int nCurNum, int nSum)
    {
        AcArray<AcDbEntity *> arrpMatrix;

        if (nCurNum < 2 || arrp.isEmpty())
            return arrpMatrix;

        AcArray<double> arrAngle;
        double dAngle = 360.0 / nSum;
        for (int i = 1; i < nCurNum; i++)
            arrAngle.append(dAngle * i);

        for (int i = 0; i < arrAngle.length(); i++)
        {
            AcArray<AcDbEntity *> arrpCopy = CopyEnt(arrp);
            RotateEnt(arrpCopy, ptBase, m_pArxConvert->ToRadian(arrAngle[i]));
            arrpMatrix.append(arrpCopy);
        }

        return arrpMatrix;
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

    void KTArxEntity::SetEntSelected(const AcDbObjectIdArray &arridEnt, bool bSeled /*= true*/, bool bHighlight /*= false*/)
    {
        //	acedSSDel(ent, ssname);		//将实体从集合中删除掉
        acedSSSetFirst(NULL, NULL); // 将选中状态清除
        if (bSeled)
        {
            // 使实体被选中
            ads_name ssname, ent;
            acdbNameClear(ssname);
            for (int i = 0; i < arridEnt.length(); ++i)
            {
                acdbGetAdsName(ent, arridEnt.at(i));
                acedSSAdd(ent, ssname, ssname); // 将实体增加到集合中
            }
            acedSSSetFirst(ssname, NULL); // 把实体变为选中状态
        }

        if (bHighlight)
        {
            for (int i = 0; i < arridEnt.length(); ++i)
            {
                AcDbEntityPointer pEnt(arridEnt.at(i), AcDb::kForWrite);
                if (pEnt.openStatus() != Acad::eOk)
                    continue;
                pEnt->highlight(); // 设置实体为高亮状态
            }
        }
    }

    bool KTArxEntity::SetEntToBottom(const AcDbObjectId &id, AcDbDatabase *pDb /*= acdbCurDwg()*/)
    {
        AcDbObjectId spaceId = pDb->currentSpaceId();
        AcDbObjectPointer<AcDbBlockTableRecord> pBTR(spaceId, AcDb::kForRead);
        if (pBTR.openStatus() != Acad::eOk)
            return false;
        AcDbSortentsTable *pSortTab = nullptr;
        AcDbObjectIdArray idarrTemp;
        idarrTemp.append(id);
        Acad::ErrorStatus es = pBTR->getSortentsTable(pSortTab, AcDb::kForWrite, true);
        if (Acad::eOk != es)
            return false;
        pSortTab->moveToBottom(idarrTemp);
        pSortTab->close();
        return true;
    }

    bool KTArxEntity::StretchEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptCorner1, const AcGePoint3d &ptCorner2, const AcGePoint3d &ptBase, const AcGePoint3d &ptTarget)
    {
        AcGeVector3d stretchVec = ptTarget - ptBase; // 拉伸距离
        AcGeBoundBlock3d box;
        box.set(ptCorner1, AcGeVector3d((ptCorner2 - ptCorner1).x, 0, 0),
                AcGeVector3d(0, (ptCorner2 - ptCorner1).y, 0),
                AcGeVector3d(0, 0, (ptCorner2 - ptCorner1).z));
        for (int i = 0; i < arrid.length(); ++i)
        {
            AcDbObjectPointer<AcDbEntity> pEnt(arrid[i], AcDb::kForWrite);
            if (Acad::eOk != pEnt.openStatus())
                continue;
            AcGePoint3dArray arrpt;
            if (Acad::eOk != pEnt->getStretchPoints(arrpt))
                continue;
            AcDbIntArray arrint;
            bool bFound = false;
            for (int j = 0; j < arrpt.length(); ++j)
            {
                AcGePoint3d pt = arrpt[j];
                if (box.contains(pt))
                {
                    arrint.append(j);
                    bFound = true;
                }
            }
            if (bFound)
                pEnt->moveStretchPointsAt(arrint, stretchVec);
        }
        return true;
    }

    AcDbObjectIdArray KTArxEntity::GetEntIdByPt(const AcGePoint3d &pt, bool bAll /*= false*/)
    {
        AcDbObjectIdArray arrid;
        ads_name ssname;

        if (bAll)
        {
            // 得到通过该点的所有实体
            if (RTNORM != acedSSGet(_T("C"), asDblArray(pt), asDblArray(pt), NULL, ssname))
                return arrid;
        }
        else
        {
            // 得到位于该点最上面的实体
            if (RTNORM != acedSSGet(NULL, asDblArray(pt), NULL, NULL, ssname))
                return arrid;
        }
        // 遍历选择集
        int lLength = 0;
        acedSSLength(ssname, &lLength);
        for (int i = 0; i < lLength; i++)
        {
            ads_name name;
            acedSSName(ssname, i, name);
            AcDbObjectId id;
            acdbGetObjectId(id, name);
            arrid.append(id);
        }
        acedSSFree(ssname);

        return arrid;
    }
} // namespace KTArxTool