#ifndef AUTHJAVASCRIPT_H
#define AUTHJAVASCRIPT_H

#include <QObject>
#include <functional>

struct TypedException;

namespace auth
{

class Auth;
struct LoginInfo;

class AuthJavascript : public QObject
{
    Q_OBJECT
public:
    using Callback = std::function<void()>;

public:
    explicit AuthJavascript(QObject *parent = nullptr);

    void setAuthManager(Auth &auth)
    {
        authManager = &auth;
    }

public slots:
    Q_INVOKABLE void login(const QString &login, const QString &password);

    Q_INVOKABLE void logout();

    Q_INVOKABLE void check();

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const Callback &callback);

signals:
    void sendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error);

    void sendLoginErrorResponseSig(const TypedException &error);

public slots:

    void onSendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error);

    void onSendLoginErrorResponseSig(const TypedException &error);

    void onCallbackCall(const Callback &callback);


private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:
    Auth *authManager;
};

}
#endif // AUTHJAVASCRIPT_H
