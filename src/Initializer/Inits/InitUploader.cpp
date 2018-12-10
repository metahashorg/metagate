#include "InitUploader.h"

#include "../Initializer.h"

#include "uploader.h"

#include "check.h"
#include "SlotWrapper.h"

namespace initializer {

InitUploader::InitUploader(QThread *mainThread, Initializer &manager)
    : QObject(nullptr)
    , InitInterface(mainThread, manager)
{
    QTimer::singleShot(milliseconds(30s).count(), [this]{
        onCheckedUpdatesHtmls(TypedException(INITIALIZER_TIMEOUT_ERROR, "uploader check updates"));
    });
}

InitUploader::~InitUploader() = default;

void InitUploader::complete() {
    CHECK(uploader != nullptr, "uploader not initialized");
    CHECK(isFlushed, "Not flushed");
}

void InitUploader::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("uploader", "init", "uploader initialized", exception));
}

void InitUploader::onCheckedUpdatesHtmls(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    if (!isFlushed) {
        sendState(InitState("uploader", "check_updates_htmls", "uploader check updates", exception));
        isFlushed = true;
    }
END_SLOT_WRAPPER
}

InitUploader::Return InitUploader::initialize(std::shared_future<MainWindow*> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        uploader = std::make_unique<Uploader>(*mainWindow.get());
        CHECK(connect(uploader.get(), &Uploader::checkedUpdatesHtmls, this, &InitUploader::onCheckedUpdatesHtmls), "not connect onCheckedUpdatesHtmls");
        uploader->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return uploader.get();
}

}
