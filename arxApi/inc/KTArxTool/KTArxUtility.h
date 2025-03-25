#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUtility
    {
    public:
        // Summary:   �õ���ǰ��Ŀzrx�ļ�·��
        CString GetCurZrxFilePath();

        // Summary:   �õ���ǰ��Ŀzrx�ļ�����Ŀ¼·��
        // Explain:   ���ص�Ŀ¼·������'\\'
        CString GetCurZrxFolderPath();

        // Summary:   ����cad�˵��ļ�
        bool LoadCadMenu(const CString &strMenuFileName, const CString &strMenuGroupName);

        // Summary:   ж��cad�˵�
        bool UnloadCadMenu(const CString &strMenuGroupName);

        // Summary:   ����zrx�ļ�
        bool LoadZrxFile(LPCTSTR szFilePath);

        // Summary:   �������·��
        bool AddSearchPath(LPCTSTR szSearchDir);

        // Summary:   ��ʵ����ӵ�ģ�Ϳռ�
        AcDbObjectId PostToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());

        /// @brief �Ե�������������
        /// @param bVertical ���������ʶ	false:��ˮƽ����	true:����ֱ����
        AcGePoint3dArray SortPtArr(const AcGePoint3dArray &PointArr, bool bVertical = false);
    };
} // namespace KTARXTOOL
