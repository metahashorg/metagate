#include "InitUploader.h"

#include "uploader.h"

#include "check.h"
#include "SlotWrapper.h"
#include "QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitUploader::stateName() {
    return "uploader";
}

InitUploader::InitUploader(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    Q_CONNECT(this, &InitUploader::checkedUpdatesHtmls, this, &InitUploader::onCheckedUpdatesHtmls);

    registerStateType("init", "uploader initialized", true, true);
    registerStateType("check_updates_htmls", "uploader check updates", false, false, 30s, "uploader check updates");
}

InitUploader::~InitUploader() = default;

void InitUploader::completeImpl() {
    CHECK(uploader != nullptr, "uploader not initialized");
}

void InitUploader::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

void InitUploader::onCheckedUpdatesHtmls(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendState("check_updates_htmls", false, exception);
END_SLOT_WRAPPER
}

InitUploader::Return InitUploader::initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth) {
    const TypedException exception = apiVrapper2([&, this] {
        uploader = std::make_unique<Uploader>(*auth.get().first, *mainWindow.get());
        Q_CONNECT(uploader.get(), &Uploader::checkedUpdatesHtmls, this, &InitUploader::checkedUpdatesHtmls);
        uploader->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return uploader.get();
}

}
