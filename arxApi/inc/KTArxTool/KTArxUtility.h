#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxUtility
    {
    public:
        /// @brief �õ���ǰ��Ŀzrx�ļ�·��
        CString GetCurZrxFilePath();

        /// @brief �õ���ǰ��Ŀzrx�ļ�����Ŀ¼·��
        /// @details ���ص�Ŀ¼·������'\\'
        CString GetCurZrxFolderPath();

        /// @brief ����cad�˵��ļ�
        bool LoadCadMenu(const CString &strMenuFileName, const CString &strMenuGroupName);

        /// @brief ж��cad�˵�
        bool UnloadCadMenu(const CString &strMenuGroupName);

        /// @brief ����zrx�ļ�
        bool LoadZrxFile(LPCTSTR szFilePath);

        /// @brief �������·��
        bool AddSearchPath(LPCTSTR szSearchDir);

        /// @brief ��ʵ����ӵ�ģ�Ϳռ�
        AcDbObjectId PostToModelSpace(AcDbEntity *pEnt, AcDbDatabase *pDb = acdbCurDwg());

        /// @brief �Ե�������������
        /// @param bVertical ���������ʶ	false:��ˮƽ����	true:����ֱ����
        AcGePoint3dArray SortPtArr(const AcGePoint3dArray &PointArr, bool bVertical = false);

        /// @brief ��ʾ/�رս�����
        void ShowProgressBar(const CString &strName, bool bStart);

        /// @brief ���ý�����
        /// @param nCurIndex    ��ǰ�±�
        /// @param nSum         �ܵ��±�����
        void SetProgressBar(int nCurIndex, int nSum);
    };
} // namespace KTARXTOOL
