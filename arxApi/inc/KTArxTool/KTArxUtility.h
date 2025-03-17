#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUtility
    {
    public:
        AcDbObjectId postToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());
    };

} // namespace KTARXTOOL
