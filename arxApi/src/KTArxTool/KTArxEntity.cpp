namespace KTArxTool
{
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