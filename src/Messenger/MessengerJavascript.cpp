#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"

#include "Wallet.h"

MessengerJavascript::MessengerJavascript(QObject *parent)
    : QObject(parent)
{
    //CHECK(connect(this, &MessengerJavascript::getHistoryAddressSig, this, &MessengerJavascript::onGetHistoryAddress), "not connect onGetHistoryAddress");
}

template<class Function>
TypedException MessengerJavascript::apiVrapper(const Function &func) {
    // TODO когда будет if constexpr, объединить обе функции в одну
    try {
        func();
        return TypedException();
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        return e;
    } catch (const Exception &e) {
        LOG << "Error " << e;
        return TypedException(TypeErrors::OTHER_ERROR, e);
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        return TypedException(TypeErrors::OTHER_ERROR, e.what());
    } catch (...) {
        LOG << "Unknown error";
        return TypedException(TypeErrors::OTHER_ERROR, "Unknown error");
    }
}

template<typename... Args>
void MessengerJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc2<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void MessengerJavascript::onGetHistoryAddress(const QString requestId, const QString address, const QString from, const QString to) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgGetHistoryAddressJs";

    LOG << "get messages";

    const Messenger::Counter fromC = std::stoull(from.toStdString());
    const Messenger::Counter toC = std::stoull(to.toStdString());
    Opt<QString> result;
    const TypedException exception = apiVrapper([&, this](){
        result = "ok";
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void MessengerJavascript::runJs(const QString &script) {
    emit jsRunSig(script);
}
