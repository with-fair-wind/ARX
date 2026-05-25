#include <Services/component_browser_doc_reactor.h>

#include <Dialog/component_browser_dialog.h>

namespace {
void* docKeyFromDocument(AcApDocument* doc) {
    if (doc == nullptr) {
        return nullptr;
    }
    if (doc->database() != nullptr) {
        return doc->database();
    }
    return doc;
}
}  // namespace

void ComponentBrowserDocReactor::documentToBeActivated(AcApDocument* /*pDocToActivate*/) {
    ZcBmComponentBrowserDialog::notifyBeforeDocumentSwitch();
}

void ComponentBrowserDocReactor::documentActivated(AcApDocument* /*pActivatedDoc*/) {
    ZcBmComponentBrowserDialog::notifyAfterDocumentSwitch();
}

void ComponentBrowserDocReactor::documentToBeDestroyed(AcApDocument* pDocToDestroy) {
    ZcBmComponentBrowserDialog::notifyDocumentToBeDestroyed(docKeyFromDocument(pDocToDestroy));
}
