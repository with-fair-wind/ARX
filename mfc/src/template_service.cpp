#include <Services/app_event_service.h>
#include <Services/template_service.h>

bool loadTemplate(const std::wstring& template_name) {
    // TODO: 实际加载族模板逻辑

    // 入队事件, commandEnded 时统一 flush 到 UI
    AppEventService::instance().post(TemplateChangedEvent{template_name});
    return true;
}

bool unloadTemplate(const std::wstring& template_name) {
    // TODO: 实际卸载族模板逻辑

    AppEventService::instance().post(TemplateChangedEvent{template_name});
    return true;
}
