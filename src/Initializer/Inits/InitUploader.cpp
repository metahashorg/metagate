#include "InitUploader.h"

#include "../Initializer.h"

#include "uploader.h"

#include <functional>
using namespace std::placeholders;

#include "check.h"
#include "SlotWrapper.h"

namespace initializer {

InitUploader::InitUploader(QThread *mainThread, Initializer &manager)
    : InitInterface(mainThread, manager, true)
{
    CHECK(connect(this, &InitUploader::checkedUpdatesHtmls, this, &InitUploader::onCheckedUpdatesHtmls), "not connect onCheckedUpdatesHtmls");
    setTimerEvent(30s, "uploader check updates", std::bind(&InitUploader::checkedUpdatesHtmls, this, _1));
}

InitUploader::~InitUploader() = default;

void InitUploader::complete() {
    CHECK(uploader != nullptr, "uploader not initialized");
    CHECK(isFlushed, "Not flushed");
}

void InitUploader::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("uploader", "init", "uploader initialized", true, exception));
}

void InitUploader::onCheckedUpdatesHtmls(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    if (!isFlushed) {
        sendState(InitState("uploader", "check_updates_htmls", "uploader check updates", false, exception));
        isFlushed = true;
    }
END_SLOT_WRAPPER
}

InitUploader::Return InitUploader::initialize(std::shared_future<MainWindow*> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        uploader = std::make_unique<Uploader>(*mainWindow.get());
        CHECK(connect(uploader.get(), &Uploader::checkedUpdatesHtmls, this, &InitUploader::checkedUpdatesHtmls), "not connect onCheckedUpdatesHtmls");
        uploader->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return uploader.get();
}

}
