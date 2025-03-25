#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUtility
    {
    public:
        // Summary:   得到当前项目zrx文件路径
        CString GetCurZrxFilePath();

        // Summary:   得到当前项目zrx文件所在目录路径
        // Explain:   返回的目录路径含带'\\'
        CString GetCurZrxFolderPath();

        // Summary:   加载cad菜单文件
        bool LoadCadMenu(const CString &strMenuFileName, const CString &strMenuGroupName);

        // Summary:   卸载cad菜单
        bool UnloadCadMenu(const CString &strMenuGroupName);

        // Summary:   加载zrx文件
        bool LoadZrxFile(LPCTSTR szFilePath);

        // Summary:   添加搜索路径
        bool AddSearchPath(LPCTSTR szSearchDir);

        // Summary:   将实体添加到模型空间
        AcDbObjectId PostToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());

        // Summary:   按比例缩放实体
        bool ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale, const AcGePoint3d &ptBase);

        // Summary:   分解实体
        AcArray<AcDbEntity *> ExplodeEnt(AcDbEntity *pEnt);
        AcArray<AcDbEntity *> ExplodeEnt(const AcDbObjectId &idEnt);

        // Summary:   移动实体
        bool MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);

        // Summary:   旋转实体(弧度)
        bool RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation);
        bool RotateEnt(const AcArray<AcDbEntity *> &arrpEnt, const AcGePoint3d &ptBase, double rotation);
    };
} // namespace KTARXTOOL
