#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUiPr
    {
    public:
        bool SelEnt(LPCTSTR szPrompt, AcRxClass *classType, AcDbObjectId &id, AcGePoint3d &pt);
    };
} // namespace KTArxUiPr
