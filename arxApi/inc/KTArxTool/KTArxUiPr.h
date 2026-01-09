#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxUiPr {
   public:
    static bool SelEnt(LPCTSTR prompt, AcRxClass* classType, AcDbObjectId& objectId, AcGePoint3d& pickedPoint);
};
}  // namespace KTArxTool
