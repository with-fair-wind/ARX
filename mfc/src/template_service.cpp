#include <Services/app_event_service.h>
#include <Services/template_service.h>

bool loadTemplate(const std::wstring& template_name) {
    // TODO: 实际加载族模板逻辑

    // 标记脏, commandEnded 时自动 flush 到 UI
    AppEventService::instance().event<TemplateChangedEvent>().markDirty(template_name);
    return true;
}

bool unloadTemplate(const std::wstring& template_name) {
    // TODO: 实际卸载族模板逻辑

    AppEventService::instance().event<TemplateChangedEvent>().markDirty(template_name);
    return true;
}
