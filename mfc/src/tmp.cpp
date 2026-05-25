#include <functional>
enum class PlacementType {
    kLine,
    kArc,
    // 预留扩展：kSpline / kPolyline
};

struct IPlacementGeometry {
    virtual ~IPlacementGeometry() = default;
};

struct LinePlacementGeometry : IPlacementGeometry {
    ZcGePoint3d start;
    ZcGePoint3d end;
};

struct ArcPlacementGeometry : IPlacementGeometry {
    ZcGePoint3d start;
    ZcGePoint3d end;
    ZcGePoint3d center;
    double radius;
};

class IPlacementStrategy {
protected:
    std::reference_wrapper<IPlacementGeometry> m_geometry;

public:
    explicit IPlacementStrategy(IPlacementGeometry& geometry) : m_geometry(geometry) {}
    virtual ~IPlacementStrategy() = default;

    virtual ZcGePoint3d MapPoint(double x, double y, double z) const = 0;
    virtual CurvePtr MapCurve(const CurvePtr& curve) const = 0;
    virtual SurfacePtr MapSurface(const SurfacePtr& surface) const = 0;
    virtual SolidPtr MapSolid(const SolidPtr& solid) const = 0;
};

class LinePlacementStrategy : public IPlacementStrategy {
public:
    explicit LinePlacementStrategy(LinePlacementGeometry& geometry) : IPlacementStrategy(geometry) {}
    // TODO
};

class ArcPlacementStrategy : public IPlacementStrategy {
public:
    explicit ArcPlacementStrategy(ArcPlacementGeometry& geometry) : IPlacementStrategy(geometry) {}
    // TODO
};

class PlacementStrategyFactory {
public:
    static std::unique_ptr<IPlacementStrategy> Create(PlacementType type, const IPlacementGeometry& geo);
};