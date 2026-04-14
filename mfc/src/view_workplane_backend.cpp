#include <Services/view_workplane_backend.h>

#include <acedads.h>
#include <acutads.h>

#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace {
bool setError(std::wstring* errorMessage, const std::wstring& text) {
    if (errorMessage != nullptr) {
        *errorMessage = text;
    }
    return false;
}

std::wstring formatPickedPoint(const AcGePoint3d& point) {
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(3) << point.x << L"," << point.y << L"," << point.z;
    return stream.str();
}

class StubViewWorkPlaneBackend final : public IViewWorkPlaneBackend {
   public:
    bool listViews(std::vector<BimViewItem>* outViews, std::wstring* errorMessage) override {
        if (outViews == nullptr) {
            return setError(errorMessage, L"输出视图列表为空。");
        }

        outViews->clear();
        outViews->push_back({L"view_plan_1f", L"1F", BimViewKind::kPlan});
        outViews->push_back({L"view_plan_2f", L"2F", BimViewKind::kPlan});
        outViews->push_back({L"view_elevation_n", L"南立面", BimViewKind::kElevation});
        outViews->push_back({L"view_elevation_e", L"东立面", BimViewKind::kElevation});
        outViews->push_back({L"view_section_1", L"1-1剖面", BimViewKind::kSection});
        outViews->push_back({L"view_3d_default", L"三维视图", BimViewKind::kThreeD});
        if (errorMessage != nullptr) {
            errorMessage->clear();
        }
        return true;
    }

    bool getAssociatedWorkPlanes(const std::vector<std::wstring>& viewIds, std::vector<ViewWorkPlaneAssociation>* outAssociations,
                                 std::wstring* errorMessage) override {
        if (outAssociations == nullptr) {
            return setError(errorMessage, L"输出关联列表为空。");
        }
        outAssociations->clear();

        static const std::unordered_map<std::wstring, WorkPlaneRef> kMockAssociations = {
            {L"view_plan_1f", {L"wp_lobby", L"首层大厅"}},
            {L"view_elevation_n", {L"wp_axis_a", L"A轴立面"}},
        };

        for (const std::wstring& viewId : viewIds) {
            const auto it = kMockAssociations.find(viewId);
            if (it == kMockAssociations.end()) {
                continue;
            }

            ViewWorkPlaneAssociation association;
            association.viewId = viewId;
            association.workPlaneId = it->second.id;
            association.workPlaneName = it->second.name;
            outAssociations->push_back(std::move(association));
        }

        if (errorMessage != nullptr) {
            errorMessage->clear();
        }
        return true;
    }

    bool pickWorkPlaneForView(const std::wstring& viewId, WorkPlaneRef* outWorkPlane, std::wstring* errorMessage) override {
        if (outWorkPlane == nullptr) {
            return setError(errorMessage, L"输出工作平面对象为空。");
        }

        AcGePoint3d pickedPoint;
        const std::wstring prompt = L"\n为视图 [" + viewId + L"] 选择一个点以模拟关联工作平面: ";
        const int status = acedGetPoint(nullptr, prompt.c_str(), asDblArray(pickedPoint));
        if (status != RTNORM) {
            if (errorMessage != nullptr) {
                errorMessage->clear();
            }
            return false;
        }

        const std::wstring pointText = formatPickedPoint(pickedPoint);
        outWorkPlane->id = L"mock_point_" + pointText;
        outWorkPlane->name = L"点(" + pointText + L")";
        if (errorMessage != nullptr) {
            errorMessage->clear();
        }
        return true;
    }

    bool setAssociatedWorkPlanes(const std::vector<ViewWorkPlaneAssociation>& associations, std::wstring* errorMessage) override {
        static_cast<void>(associations);
        if (errorMessage != nullptr) {
            errorMessage->clear();
        }
        return true;
    }
};
}  // namespace

std::shared_ptr<IViewWorkPlaneBackend> createViewWorkPlaneBackend() { return std::make_shared<StubViewWorkPlaneBackend>(); }
