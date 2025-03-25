#pragma once

namespace KTArxTool
{
    class KT_EXPORTARX_API KTArxCurve
    {
    public:
        /// @brief   �õ������������߶����ҵ�����ֵ
        /// @details ptBase �� vecDir ƫ�ƺ��Ƿ��������ߵġ�����
        bool GetPosAngNegValue(AcDbCurve *pCurve, const AcGePoint3d &ptBase, const AcGeVector3d &vecDir);

        /// @brief ƫ���߶�
        AcArray<AcDbCurve *> OffsetCurve(AcDbCurve *pCurve, double dOffset);
        /// @details ͨ�� pt �Զ�����ƫ�Ʒ���
        AcArray<AcDbCurve *> OffsetCurve(AcDbCurve *pCurve, double dOffset, const AcGePoint3d &pt, bool bOpposite = false);

        /// @brief �õ��߶��Ͼ���õ�����ĵ�
        AcGePoint3d GetClosedPtInCurve(AcDbCurve *pCurve, const AcGePoint3d &pt);
        bool GetClosedPtInCurve(const AcDbObjectId &idCurve, const AcGePoint3d &pt, AcGePoint3d &ptClosed);

        /// @brief ����߶�
        /// @details ����ĵ����Ҫ��getClosestPointTo�������»��
        AcArray<AcDbCurve *> SplitCurve(AcDbCurve *pCurve, const AcGePoint3dArray &arrSplitPt);
        AcArray<AcDbCurve *> SplitCurve(const AcDbObjectId &idCurve, const AcGePoint3dArray &arrSplitPt);
        AcArray<AcDbCurve *> SplitCurve(AcDbCurve *pCurve, const AcGePoint3d &ptSplit);

        /// @brief ͨ�����������߶�
        bool ExtendCurveByDist(AcDbPolyline *pPline, bool isStartPt, double dDist);
        /// @brief �����߶�
        /// @details ��չһ������(pExtend), ʹ������һ���߽�����(pBorder)�ཻ
        bool ExtendCurve(AcDbCurve *pBorder, AcDbCurve *pExtend, const AcGePoint3d &ptExtend);

        /// @brief �޼��߶�
        /// @details �޼�һ������(pTrim), ʹ������һ���߽�����(pBorder)�ཻ, �޼��� ptTrim, �޼���������� pNewTrim
        bool TrimCurve(AcDbCurve *pBorder, AcDbCurve *pTrim, const AcGePoint3d &ptTrim, AcDbCurve *&pNewTrim);

        /// @brief �жϵ��Ƿ����߶���
        bool IsPtOnCurve(const AcGePoint3d &pt, AcDbCurve *pCurve, double dTol = 0);

        /// @brief �õ�Բ��͹��
        double GetArcBulge(AcDbArc *pArc);
        double GetArcBulge(double dAngleStart, double dAngleEnd);
        double GetArcBulge(const AcDbObjectId &idArc);
        double GetArcBulge(AcDbCurve *pCurve);
        double GetArcBulge(const AcGeCircArc2d &geArc);

        /// @brief �õ����������
        AcGeLine2d GetGeLine2d(AcDbLine *pLine);
        AcGeLineSeg2d GetGeLineSeg2d(AcDbLine *pLine);
        AcGeLineSeg2d GetGeLineSeg2d(const AcDbObjectId &idLine);
        AcGeLineSeg2d GetGeLineSeg2d(AcDbPolyline *pPolyline, unsigned int unIndex);
        AcGeCircArc2d GetGeCircArc2d(AcDbArc *pArc);
        AcGeCircArc2d GetGeCircArc2d(AcDbCircle *pCircle);
        AcGeCircArc2d GetGeCircArc2d(AcDbPolyline *pPolyline, unsigned int unIndex);

        /// @brief �õ�����
        bool GetIntersectPoint(const AcGeLine2d &geLine1, const AcGeLine2d &geLine2,
                               AcGePoint3d &ptIntersect);
        bool GetIntersectPoint(const AcGeLineSeg2d &geLine1, const AcGeLineSeg2d &geLine2,
                               AcGePoint3d &ptIntersect);
        bool GetIntersectPoint(const AcGeLineSeg2d &geLine1, const AcGeLine2d &geLine2,
                               AcGePoint3d &ptIntersect);
        bool GetIntersectPoint(const AcGeCircArc2d &geCircArc1, const AcGeCircArc2d &geCircArc2,
                               int &nNum, AcGePoint3dArray &arrptIntersect);
        bool GetIntersectPoint(const AcGeLine2d &geLine, const AcGeCircArc2d &geCircArc,
                               int &nNum, AcGePoint3dArray &arrptIntersect);
        bool GetIntersectPoint(const AcGeLineSeg2d &geLine, const AcGeCircArc2d &geCircArc,
                               int &nNum, AcGePoint3dArray &arrptIntersect);

        /// @brief �õ���������һ�����ϵ�ͶӰ����
        /// @param ptBase 	    �������
        /// @param vctOffset    ���뱻ͶӰ������
        /// @param vctDirection ���뷽������
        /// @return
        AcGeVector3d GetProjectVector(const AcGePoint3d &ptBase, const AcGeVector3d &vctOffset, const AcGeVector3d &vctDirection);

        /// @brief �õ��߶�ĳ�����������
        AcGeVector3d GetTangentLineVec(const AcGePoint3d &ptOnCurve, const AcDbObjectId &idCurve);
        bool GetTangentLineVecArrForPolyline(AcDbPolyline *pPline, const AcGePoint3d &pt, AcGeVector3dArray &arrvec);
        AcGeVector3dArray GetTangentLineVecArr(const AcGePoint3d &pt, AcDbCurve *pCurve);
        AcGeVector3dArray GetTangentLineVecArr(const AcGePoint3d &pt, const AcDbObjectId &idCurve);

        /// @brief �õ���ֱ�ߵĽ�ƽ��������
        /// @param pLine1 ����ֱ��1��id
        /// @param pLine2 ����ֱ��2��id
        /// @return �����ƽ��������
        AcGeVector2d GetMidVectorOfLines(AcDbLine *pLine1, AcDbLine *pLine2);

        /// @brief ͨ�������͵�õ���X��ĽǶ�(0~2��)
        double GetAngle(const AcGeVector3d &vec);

        /// @brief �õ��߶γ���
        double GetCurveLength(AcDbCurve *pCurve);

        /// @brief �õ���ֱ�ߵļн�
        double GetAngleOfLines(AcDbLine *pLine1, AcDbLine *pLine2);

    private:
        std::shared_ptr<KTArxEntity> m_pArxEntity;
        std::shared_ptr<KTArxConvert> m_pArxConvert;
    };
} // namespace KTArxTool
