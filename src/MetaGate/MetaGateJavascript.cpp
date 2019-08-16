#include "MetaGateJavascript.h"

#include "MetaGate.h"

#include "Log.h"
#include "check.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/WrapperJavascriptImpl.h"

#include "Network/NetworkTestingTestResult.h"
#include "NsLookup/NsLookupStructs.h"

#include <QJsonObject>
#include <QJsonArray>

SET_LOG_NAMESPACE("MG");

namespace metagate {

MetaGateJavascript::MetaGateJavascript(MetaGate &metagate)
    : WrapperJavascript(false, LOG_FILE)
    , metagate(metagate)
{
    Q_CONNECT(&metagate, &MetaGate::metaOnlineResponse, this, &MetaGateJavascript::onMetaOnlineResponse);
    Q_CONNECT(&metagate, &MetaGate::showExchangePopup, this, &MetaGateJavascript::onShowExchangePopup);
}

static QJsonDocument makeNetworkStatusResponse(const std::vector<NetworkTestingTestResult> &networkTestsResults, const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError) {
    QJsonObject result;
    QJsonArray networkTestsJson;
    for (const NetworkTestingTestResult &r: networkTestsResults) {
        QJsonObject rJson;
        rJson.insert("node", r.host);
        rJson.insert("isTimeout", r.isTimeout);
        rJson.insert("timeMs", r.timeMs);
        networkTestsJson.push_back(rJson);
    }
    result.insert("networkTests", networkTestsJson);

    QJsonObject dnsErrorJson;
    if (!dnsError.isEmpty()) {
        dnsErrorJson.insert("node", dnsError.dnsName);
    }
    result.insert("dnsErrors", dnsErrorJson);

    QJsonArray nodesStatusesJson;
    for (const NodeTypeStatus &st: nodeStatuses) {
        QJsonObject stJson;
        stJson.insert("node", st.node);
        stJson.insert("countWorked", (int)st.countWorked);
        stJson.insert("countAll", (int)st.countAll);
        stJson.insert("bestTime", (int)st.bestResult);
        nodesStatusesJson.push_back(stJson);
    }
    result.insert("dnsStats", nodesStatusesJson);
    return QJsonDocument(result);
}

void MetaGateJavascript::updateAndReloadApplication(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>("Not ok"));

    LOG << "Update and reload app ";

    wrapOperation([&, this](){
        emit metagate.updateAndReloadApplication(metagate::MetaGate::UpdateAndReloadApplicationCallback([makeFunc](bool result){
            makeFunc.func(TypedException(), (result ? "Ok" : "Not ok"));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::exitApplication() {
BEGIN_SLOT_WRAPPER
    LOG << "Exit ";
    emit metagate.exitApplication();
END_SLOT_WRAPPER
}

void MetaGateJavascript::restartBrowser() {
BEGIN_SLOT_WRAPPER
    LOG << "Restart browser ";
    emit metagate.restartBrowser();
END_SLOT_WRAPPER
}

void MetaGateJavascript::getAppInfoCallback(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>(""), JsTypeReturn<bool>(true), JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit metagate.getAppInfo(metagate::MetaGate::GetAppInfoCallback([makeFunc](const QString &hardwareId, bool isProductionSetup, const QString &versionString, const QString &gitHash){
            makeFunc.func(TypedException(), hardwareId, isProductionSetup, versionString, gitHash);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::lineEditReturnPressed(const QString &text) {
BEGIN_SLOT_WRAPPER
    emit metagate.lineEditReturnPressed(text);
END_SLOT_WRAPPER
}

void MetaGateJavascript::metaOnline(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>("Not ok"));

    LOG << "MetaOnline ";

    wrapOperation([&, this](){
        emit metagate.metaOnline(metagate::MetaGate::MetaOnlineCallback([makeFunc](bool result){
            makeFunc.func(TypedException(), (result ? "Ok" : "Not ok"));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::clearNsLookup(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>("Not ok"));

    LOG << "Clear ns lookup ";

    wrapOperation([&, this](){
        emit metagate.clearNsLookup(metagate::MetaGate::ClearNsLookupCallback([makeFunc](bool result){
            makeFunc.func(TypedException(), (result ? "Ok" : "Not ok"));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::sendMessageToWss(const QString &message, const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>("Not ok"));

    LOG << "Send message to wss " << message;

    wrapOperation([&, this](){
        emit metagate.sendMessageToWss(message, metagate::MetaGate::SendMessageToWssCallback([makeFunc](bool result){
            makeFunc.func(TypedException(), (result ? "Ok" : "Not ok"));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::setForgingActive(bool isActive, const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>("Not ok"));

    LOG << "Set forging is active " << isActive;

    wrapOperation([&, this](){
        emit metagate.setForgingActive(isActive, metagate::MetaGate::SetForgingActiveCallback([makeFunc](bool result){
            makeFunc.func(TypedException(), (result ? "Ok" : "Not ok"));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::getForgingIsActive(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<bool>(false));

    wrapOperation([&, this](){
        emit metagate.getForgingIsActive(metagate::MetaGate::GetForgingActiveCallback([makeFunc](bool result){
            LOG << "Get forging is active " << result;

            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::getNetworkStatus(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    LOG << "Get network status";

    wrapOperation([&, this](){
        emit metagate.getNetworkStatus(metagate::MetaGate::GetNetworkStatusCallback([makeFunc](const std::vector<NetworkTestingTestResult> &networkTestsResults, const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError){
            makeFunc.func(TypedException(), makeNetworkStatusResponse(networkTestsResults, nodeStatuses, dnsError));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MetaGateJavascript::onMetaOnlineResponse(const QString &response) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "onlineResultJs";

    LOG << "Meta online response: " << response;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), response);
END_SLOT_WRAPPER
}

void MetaGateJavascript::onShowExchangePopup(const QString &type) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "showExchangePopUpJs";

    LOG << "EVENT: " << type;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), type);
END_SLOT_WRAPPER
}

} // namespace metagate
