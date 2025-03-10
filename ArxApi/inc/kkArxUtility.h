namespace KK
{
    class KK_OBJECTARX_API KKArxUtility
    {
    public:
        AcDbObjectId postToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());
    };

} // namespace KK
