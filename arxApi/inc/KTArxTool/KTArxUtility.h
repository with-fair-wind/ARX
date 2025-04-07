#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUtility
    {
    public:
        /// @brief 得到当前项目zrx文件路径
        CString GetCurZrxFilePath();

        /// @brief 得到当前项目zrx文件所在目录路径
        /// @details 返回的目录路径含带'\\'
        CString GetCurZrxFolderPath();

        /// @brief 加载cad菜单文件
        bool LoadCadMenu(const CString &strMenuFileName, const CString &strMenuGroupName);

        /// @brief 卸载cad菜单
        bool UnloadCadMenu(const CString &strMenuGroupName);

        /// @brief 加载zrx文件
        bool LoadZrxFile(LPCTSTR szFilePath);

        /// @brief 添加搜索路径
        bool AddSearchPath(LPCTSTR szSearchDir);

        /// @brief 将实体添加到模型空间
        AcDbObjectId PostToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());

        /// @brief 对点的数组进行排序
        /// @param bVertical 输入排序标识	false:按水平排序	true:按垂直排序
        AcGePoint3dArray SortPtArr(const AcGePoint3dArray &PointArr, bool bVertical = false);

        /// @brief 显示/关闭进度条
        void ShowProgressBar(const CString &strName, bool bStart);

        /// @brief 设置进度条
        /// @param nCurIndex    当前下标
        /// @param nSum         总的下标数量
        void SetProgressBar(int nCurIndex, int nSum);
    };
} // namespace KTARXTOOL
