#include "InitUploader.h"

#include "../Initializer.h"

#include "uploader.h"

#include "check.h"
#include "SlotWrapper.h"

namespace initializer {

InitUploader::InitUploader(QThread *mainThread, Initializer &manager)
    : QObject(nullptr)
    , InitInterface(mainThread, manager)
{}

InitUploader::~InitUploader() = default;

void InitUploader::complete() {
    CHECK(uploader != nullptr, "uploader not initialized");
    CHECK(isFlushed, "Not flushed");
}

void InitUploader::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("uploader", "init", "uploader initialized", exception));
}

void InitUploader::sendCheckedUpdatesHtmls() {
    if (!isFlushed) {
        sendState(InitState("uploader", "check_updates", "uploader check updates", TypedException()));
        isFlushed = true;
    }
}

void InitUploader::onCheckedUpdatesHtmls() {
BEGIN_SLOT_WRAPPER
    sendCheckedUpdatesHtmls();
END_SLOT_WRAPPER
}

InitUploader::Return InitUploader::initialize(std::shared_future<MainWindow*> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        uploader = std::make_unique<Uploader>(*mainWindow.get());
        CHECK(connect(uploader.get(), &Uploader::checkedUpdatesHtmls, this, &InitUploader::onCheckedUpdatesHtmls), "not connect onCheckedUpdatesHtmls");
        if (uploader->isCheckedUpdatesHtmls()) { // Так как сигнал мог прийти до коннекта, проверим здесь
            sendCheckedUpdatesHtmls();
        }
        uploader->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return uploader.get();
}

}
