#pragma once

#include <acdocman.h>

// 文档切换反应器: 将文档激活前后事件转发到构件浏览器。
class ComponentBrowserDocReactor : public AcApDocManagerReactor {
   public:
    void documentToBeActivated(AcApDocument* pDocToActivate) override;
    void documentActivated(AcApDocument* pActivatedDoc) override;
    void documentToBeDestroyed(AcApDocument* pDocToDestroy) override;
};
