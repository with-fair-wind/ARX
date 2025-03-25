#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxEntity
    {
    public:
#pragma region polyline
        /// @brief 判断该多段线是否是闭合多段线
        bool IsClosedPline(AcDbPolyline *pPline);
#pragma endregion
    };
} // namespace KTArxTool
