#include "UtilsJavascript.h"

#include "WrapperJavascriptImpl.h"

#include "Utils/Utils.h"

#include "SlotWrapper.h"

SET_LOG_NAMESPACE("UTIL");

namespace utils {

UtilsJavascript::UtilsJavascript(Utils &manager)
    : WrapperJavascript(false, LOG_FILE)
    , manager(manager)
{

}

void UtilsJavascript::qtOpenInBrowser(const QString &url) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsOpenInBrowserJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    LOG << "Open another url " << url;

    wrapOperation([&, this](){
        emit manager.openInBrowser(url, Utils::OpenInBrowserCallback([makeFunc]{
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::openFolderDialog(const QString &beginPath, const QString &caption) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsOpenFolderDialogJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    LOG << "Open folder dialog " << beginPath << " " << caption;

    wrapOperation([&, this](){
        emit manager.openFolderDialog(beginPath, caption, Utils::OpenFolderDialogCallback([makeFunc](const QString &path){
            makeFunc.func(TypedException(), path);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::saveFileFromUrl(const QString &url, const QString &saveFileWindowCaption, bool openAfterSave, const QString &filePath) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsSaveFileFromUrlJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    LOG << "Save file from url " << saveFileWindowCaption << " " << filePath << " " << openAfterSave;

    wrapOperation([&, this](){
        emit manager.saveFileFromUrl(url, saveFileWindowCaption, filePath, openAfterSave, Utils::SaveFileFromUrlCallback([makeFunc](){
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::printUrl(const QString &url, const QString &printWindowCaption, const QString &text) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsPrintUrlJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    LOG << "Print file from url " << printWindowCaption;

    wrapOperation([&, this](){
        emit manager.printUrl(url, printWindowCaption, text, Utils::PrintUrlCallback([makeFunc](){
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::chooseFileAndLoad2(const QString &openFileWindowCaption, const QString &filePath) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsChooseFileAndLoad2UrlJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<std::string>(""));

    LOG << "Choose file " << filePath;

    wrapOperation([&, this](){
        emit manager.chooseFileAndLoad(openFileWindowCaption, filePath, Utils::ChooseFileAndLoadCallback([makeFunc](const std::string &result){
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::qrEncode(const QString &textHex) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsQrEncodeJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    LOG << "Qr encode ";

    wrapOperation([&, this](){
        emit manager.qrEncode(textHex, Utils::QrEncodeCallback([makeFunc](const QString &result){
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::qrDecode(const QString &imageBase64) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "utilsQrDecodeJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    LOG << "Qr decode ";

    wrapOperation([&, this](){
        emit manager.qrDecode(imageBase64, Utils::QrDecodeCallback([makeFunc](const QString &result){
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void UtilsJavascript::javascriptLog(const QString &message) {
BEGIN_SLOT_WRAPPER
    emit manager.javascriptLog(message);
END_SLOT_WRAPPER
}

} // namespace utils
