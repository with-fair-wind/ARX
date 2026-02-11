#pragma once

#include <Services/app_event_service.h>

/// AcEditorReactor 子类: 在 commandEnded 时 flush 延迟事件
class AppEditorReactor : public AcEditorReactor {
   public:
    void commandEnded(const ACHAR* /*cmdStr*/) override {
        AppEventService::instance().flush();
    }
};
