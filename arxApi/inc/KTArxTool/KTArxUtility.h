#pragma once

namespace KTArxTool {
class KT_EXPORTARX_API KTArxUtility {
   public:
    /// @brief 得到当前项目zrx文件路径
    static CString getCurZrxFilePath();

    /// @brief 得到当前项目zrx文件所在目录路径
    /// @details 返回的目录路径含带'\\'
    static CString getCurZrxFolderPath();

    /// @brief 得到目录下所有的dwg图纸路径
    static AcArray<CString> getAllDwgPath(LPCTSTR szFolderPath);

    /// @brief 得到目录下所有的dwg图纸名
    static AcArray<CString> getAllDwgName(LPCTSTR szFolderPath);

    /// @brief 加载cad菜单文件
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    static bool loadCadMenu(const CString& strMenuFileName, const CString& strMenuGroupName);

    /// @brief 卸载cad菜单
    static bool unloadCadMenu(const CString& strMenuGroupName);

    /// @brief 加载zrx文件
    static bool loadZrxFile(LPCTSTR szFilePath);

    /// @brief 添加搜索路径
    static bool addSearchPath(LPCTSTR szSearchDir);

    /// @brief 将实体添加到模型空间
    static AcDbObjectId postToModelSpace(AcDbEntity* pEnt, AcDbDatabase* pDb = acdbCurDwg());

    /// @brief 对点的数组进行排序
    /// @param bVertical 输入排序标识	false:按水平排序	true:按垂直排序
    static AcGePoint3dArray sortPtArr(const AcGePoint3dArray& PointArr, bool bVertical = false);

    /// @brief 显示/关闭进度条
    static void showProgressBar(const CString& strName, bool bStart);

    /// @brief 设置进度条
    /// @param nCurIndex 当前下标
    /// @param nSum 总的下标数量
    static void setProgressBar(int nCurIndex, int nSum);
};
}  // namespace KTArxTool
