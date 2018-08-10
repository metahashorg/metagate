#ifndef MESSENGERJAVASCRIPT_H
#define MESSENGERJAVASCRIPT_H

#include <QObject>

#include "TypedException.h"
#include "Messenger.h"

class MessengerJavascript : public QObject {
    Q_OBJECT
public:
    explicit MessengerJavascript(QObject *parent = nullptr);

signals:

    void jsRunSig(QString jsString);

public slots:

    Q_INVOKABLE void onGetHistoryAddress(const QString requestId, const QString address, const QString from, const QString to);

private:

    template<class Function>
    TypedException apiVrapper(const Function &func);

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

};

#endif // MESSENGERJAVASCRIPT_H
