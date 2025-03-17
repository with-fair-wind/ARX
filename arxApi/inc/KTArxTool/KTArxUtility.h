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

        // Summary:   ����������ʵ��
        bool ModifyEntByScale(AcDbEntity *pEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcDbObjectId &idEnt, double dScale, const AcGePoint3d &ptBase);
        bool ModifyEntByScale(const AcArray<AcDbEntity *> &arrpEnt, double dScale, const AcGePoint3d &ptBase);

        // Summary:   �ֽ�ʵ��
        AcArray<AcDbEntity *> ExplodeEnt(AcDbEntity *pEnt);
        AcArray<AcDbEntity *> ExplodeEnt(const AcDbObjectId &idEnt);

        // Summary:   �ƶ�ʵ��
        bool MoveEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectIdArray &arrid, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcDbObjectId &idEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);
        bool MoveEnt(const AcArray<AcDbEntity *> &arrEnt, const AcGePoint3d &ptBase, const AcGePoint3d &ptDest);

        // Summary:   ��תʵ��(����)
        bool RotateEnt(AcDbEntity *pEnt, const AcGePoint3d &ptBase, double rotation);
        bool RotateEnt(const AcArray<AcDbEntity *> &arrpEnt, const AcGePoint3d &ptBase, double rotation);

        // Summary:   �õ������������߶����ҵ�����ֵ
        // Explain:	  ptBase �� vecDir ƫ�ƺ��Ƿ��������ߵġ�����
        bool GetPosAngNegValue(AcDbCurve *pCurve, const AcGePoint3d &ptBase, const AcGeVector3d &vecDir);

        // Summary:   ƫ���߶�
        AcArray<AcDbCurve *> OffsetCurve(AcDbCurve *pCurve, double dOffset);
        // Explain:	  ͨ�� pt �Զ�����ƫ�Ʒ���
        AcArray<AcDbCurve *> OffsetCurve(AcDbCurve *pCurve, double dOffset, const AcGePoint3d &pt, bool bOpposite = false);

        // Summary:   �õ��߶��Ͼ���õ�����ĵ�
        AcGePoint3d GetClosedPtInCurve(AcDbCurve *pCurve, const AcGePoint3d &pt);
        bool GetClosedPtInCurve(const AcDbObjectId &idCurve, const AcGePoint3d &pt, AcGePoint3d &ptClosed);

        // Summary:   ����߶�
        // Explain:	  ����ĵ����Ҫ��getClosestPointTo�������»��
        AcArray<AcDbCurve *> SplitCurve(AcDbCurve *pCurve, const AcGePoint3dArray &arrSplitPt);
        AcArray<AcDbCurve *> SplitCurve(const AcDbObjectId &idCurve, const AcGePoint3dArray &arrSplitPt);
        AcArray<AcDbCurve *> SplitCurve(AcDbCurve *pCurve, const AcGePoint3d &ptSplit);

        // Summary:   ͨ�����������߶�
        bool ExtendCurveByDist(AcDbPolyline *pPline, bool isStartPt, double dDist);
        // Summary:   �����߶�
        // Explain:   ��չһ������(pExtend), ʹ������һ���߽�����(pBorder)�ཻ
        bool ExtendCurve(AcDbCurve *pBorder, AcDbCurve *pExtend, const AcGePoint3d &ptExtend);

        // Summary:   �޼��߶�
        // Explain:   �޼�һ������(pTrim), ʹ������һ���߽�����(pBorder)�ཻ, �޼��� ptTrim, �޼���������� pNewTrim
        bool TrimCurve(AcDbCurve *pBorder, AcDbCurve *pTrim, const AcGePoint3d &ptTrim, AcDbCurve *&pNewTrim);

        // Summary:   �жϵ��Ƿ����߶���
        bool IsPtOnCurve(const AcGePoint3d &pt, AcDbCurve *pCurve, double dTol = 0);
    };
} // namespace KTARXTOOL
