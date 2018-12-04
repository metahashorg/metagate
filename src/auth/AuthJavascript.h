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
    explicit AuthJavascript(QThread *mainThread, QObject *parent = nullptr);

    Auth *authManager() const {return m_authManager; }

    void setAuthManager(Auth &auth)
    {
        m_authManager = &auth;
    }

public slots:
    Q_INVOKABLE void login(const QString &login, const QString &password);

    Q_INVOKABLE void logout();

    Q_INVOKABLE void check();

    Q_INVOKABLE void forceRefresh();

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const Callback &callback);

signals:
    void sendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error);

public slots:

    void onSendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error);

    void onCallbackCall(const Callback &callback);


private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:
    Auth *m_authManager;
};

}
#endif // AUTHJAVASCRIPT_H
