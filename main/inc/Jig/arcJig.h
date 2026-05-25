#pragma once

#include <Entity/arcEntity.h>

class ArcPlaceJig final : public AcEdJig {
   public:
    ArcPlaceJig() = default;
    ArcPlaceJig(const ArcPlaceJig&) = delete;
    ArcPlaceJig& operator=(const ArcPlaceJig&) = delete;
    ArcPlaceJig(ArcPlaceJig&&) = delete;
    ArcPlaceJig& operator=(ArcPlaceJig&&) = delete;
    ~ArcPlaceJig() override;

    AcDbObjectIdArray doIt();

    DragStatus sampler() override;
    ZSoft::Boolean update() override;
    AcDbEntity* entity() const override { return static_cast<AcDbEntity*>(m_pEnt); }

   private:
    enum class Phase : std::uint8_t { kDragEnd, kDragThird };

    ArcEntity* m_pEnt = nullptr;
    Phase m_phase = Phase::kDragEnd;
    bool m_validArc = false;
    AcGePoint3d m_ptStart;
    AcGePoint3d m_ptEnd;
    AcGePoint3d m_ptThird;
};
