#ifndef CALLBACKCALLWRAPPER_H
#define CALLBACKCALLWRAPPER_H

#include <QObject>

#include <functional>

class CallbackCallWrapper : public QObject {
    Q_OBJECT
public:

    using Callback = std::function<void()>;

public:
    explicit CallbackCallWrapper(QObject *parent = nullptr);

    virtual ~CallbackCallWrapper();

signals:

    void callbackCall(const CallbackCallWrapper::Callback &callback);

private slots:

    void onCallbackCall(const CallbackCallWrapper::Callback &callback);

protected:

    std::function<void(const std::function<void()> &callback)> signalFunc;
};

#endif // CALLBACKCALLWRAPPER_H
