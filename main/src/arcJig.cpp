#include <Entity/arcEntity.h>
#include <Jig/arcJig.h>


ArcPlaceJig::~ArcPlaceJig() {
    delete m_pEnt;
}

AcDbObjectIdArray ArcPlaceJig::doIt() {
    AcDbObjectIdArray ids;

    while (true) {
        if (acedGetPoint(nullptr,
                _T("\n指定弧线起点: "),
                asDblArray(m_ptStart)) != RTNORM) {
            break;
        }

        m_pEnt = new ArcEntity;
        m_phase = Phase::kDragEnd;
        m_ptEnd = m_ptStart;

        setDispPrompt(_T("\n指定弧线终点: "));
        DragStatus status = drag();

        if (status != kNormal) {
            delete m_pEnt;
            m_pEnt = nullptr;
            break;
        }

        if (m_ptStart.isEqualTo(m_ptEnd)) {
            delete m_pEnt;
            m_pEnt = nullptr;
            continue;
        }

        m_phase = Phase::kDragThird;
        m_ptThird = m_ptEnd;

        setDispPrompt(_T("\n指定弧上的点: "));
        status = drag();

        if (status == kNormal) {
            if (!m_validArc) {
                acutPrintf(_T("\n三点共线，无法构成有效圆弧。"));
                delete m_pEnt;
                m_pEnt = nullptr;
                continue;
            }
            AcDbObjectId entId = append();
            if (entId.isValid()) {
                ids.append(entId);
            }
            m_pEnt = nullptr;
        } else {
            delete m_pEnt;
            m_pEnt = nullptr;
            break;
        }
    }

    return ids;
}

AcEdJig::DragStatus ArcPlaceJig::sampler() {
    setUserInputControls(
        static_cast<UserInputControls>(
            static_cast<unsigned>(kAccept3dCoordinates)
            | static_cast<unsigned>(kNullResponseAccepted)));

    AcGePoint3d point;
    DragStatus stat = acquirePoint(point);
    if (stat != kNormal) {
        return stat;
    }

    AcGePoint3d& target = (m_phase == Phase::kDragEnd) ? m_ptEnd : m_ptThird;

    if (point.isEqualTo(target)) {
        return kNoChange;
    }

    target = point;
    return kNormal;
}

ZSoft::Boolean ArcPlaceJig::update() {
    if (m_pEnt == nullptr) {
        return Adesk::kFalse;
    }

    if (m_phase == Phase::kDragEnd) {
        m_pEnt->setVisibility(ZcDb::kInvisible);
        return Adesk::kTrue;
    }

    AcGeCircArc3d arc;
    AcGeError err = AcGe::kOk;
    arc.set(m_ptStart, m_ptThird, m_ptEnd, err);

    if (err == AcGe::kOk) {
        m_pEnt->setArc(arc);
        m_pEnt->setVisibility(ZcDb::kVisible);
        m_validArc = true;
    } else {
        m_pEnt->setVisibility(ZcDb::kInvisible);
        m_validArc = false;
    }

    return Adesk::kTrue;
}
