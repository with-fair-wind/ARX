#include <Services/property_permission_backend.h>
#include <acutads.h>

#include <algorithm>
#include <vector>

namespace {
std::vector<PropertyPermissionItem>& propertyStore() {
    static std::vector<PropertyPermissionItem> store = {
        {L"prop_wall_thickness", L"墙厚", false},
        {L"prop_width", L"宽度", false},
        {L"prop_hole_depth", L"洞深", false},
        {L"prop_height", L"高度", false},
        {L"prop_door_sill_height", L"门槛高", false},
        {L"prop_visible_state", L"可见性状态", true},
        {L"prop_fire_rating", L"耐火极限", false},
        {L"prop_insulation_level", L"保温等级", false},
        {L"prop_sound_insulation", L"隔声性能", false},
        {L"prop_surface_material", L"表面材质", true},
        {L"prop_surface_color", L"表面颜色", true},
        {L"prop_surface_gloss", L"表面光泽度", false},
        {L"prop_open_direction", L"开启方向", true},
        {L"prop_open_angle", L"最大开启角度", false},
        {L"prop_hardware_set", L"五金套件型号", false},
        {L"prop_install_elevation", L"安装基准标高", false},
        {L"prop_reference_level", L"参考楼层", true},
        {L"prop_host_wall_offset", L"宿主墙偏移距离", false},
        {L"prop_instance_comment", L"实例注释", true},
        {L"prop_type_comment", L"类型注释", true},
        {L"prop_phase_created", L"创建阶段", false},
        {L"prop_phase_demolished", L"拆除阶段", false},
        {L"prop_export_ifc_type", L"导出IFC类型映射", false},
        {L"prop_cost_code", L"成本编码", false},
        {L"prop_maintenance_cycle", L"维护周期(月)", false},
        {L"prop_manufacturer", L"制造商", true},
        {L"prop_model_number", L"产品型号", true},
        {L"prop_warranty_period", L"质保期(年)", false},
        {L"prop_extremely_long_001", L"这是一个很长很长的属性名称用于观察列表列宽不足时的显示效果_001", false},
        {L"prop_extremely_long_002", L"这是一个很长很长的属性名称用于观察列表列宽不足时的显示效果_002_包含更多描述信息", true},
        {L"prop_extremely_long_003", L"超长属性名_门窗幕墙系统参数_构件表达式与约束条件联合控制字段", false},
        {L"prop_extremely_long_004", L"超长属性名_用于测试是否出现裁剪省略以及鼠标悬停提示文本行为", true},
    };
    return store;
}

void setError(std::wstring* error_message, const std::wstring& message) {
    if (error_message != nullptr) {
        *error_message = message;
    }
}
}  // namespace

std::vector<PropertyPermissionItem> listPropertyPermissions() { return propertyStore(); }

bool setPropertyEditable(const std::wstring& property_id, bool editable, std::wstring* error_message) {
    auto& store = propertyStore();
    const auto it = std::find_if(store.begin(), store.end(), [&property_id](const PropertyPermissionItem& item) { return item.id == property_id; });
    if (it == store.end()) {
        setError(error_message, L"未找到目标属性。");
        return false;
    }

    // TODO: 替换为真实属性类接口调用。
    it->editable = editable;
    return true;
}

bool savePropertyPermissions(std::wstring* /*error_message*/) {
    // TODO: 接入真实持久化（配置文件/数据库）。
    acutPrintf(_T("\n[属性权限] savePropertyPermissions: 当前为占位实现。"));
    return true;
}
