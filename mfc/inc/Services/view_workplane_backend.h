#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class BimViewKind : std::int8_t {
    kUnknown = -1,
    kPlan = 0,
    kElevation = 1,
    kSection = 2,
    kThreeD = 3,
};

struct BimViewItem {
    std::wstring id;
    std::wstring name;
    BimViewKind kind = BimViewKind::kUnknown;
};

struct WorkPlaneRef {
    std::wstring id;
    std::wstring name;
};

struct ViewWorkPlaneAssociation {
    std::wstring viewId;
    std::wstring workPlaneId;
    std::wstring workPlaneName;
};

class IViewWorkPlaneBackend {
   public:
    IViewWorkPlaneBackend() = default;
    IViewWorkPlaneBackend(const IViewWorkPlaneBackend&) = default;
    IViewWorkPlaneBackend& operator=(const IViewWorkPlaneBackend&) = default;
    IViewWorkPlaneBackend(IViewWorkPlaneBackend&&) = default;
    IViewWorkPlaneBackend& operator=(IViewWorkPlaneBackend&&) = default;
    virtual ~IViewWorkPlaneBackend() = default;

    virtual bool listViews(std::vector<BimViewItem>* outViews, std::wstring* errorMessage) = 0;
    virtual bool getAssociatedWorkPlanes(const std::vector<std::wstring>& viewIds, std::vector<ViewWorkPlaneAssociation>* outAssociations,
                                         std::wstring* errorMessage) = 0;
    virtual bool pickWorkPlaneForView(const std::wstring& viewId, WorkPlaneRef* outWorkPlane, std::wstring* errorMessage) = 0;
    virtual bool setAssociatedWorkPlanes(const std::vector<ViewWorkPlaneAssociation>& associations, std::wstring* errorMessage) = 0;
};

std::shared_ptr<IViewWorkPlaneBackend> createViewWorkPlaneBackend();
