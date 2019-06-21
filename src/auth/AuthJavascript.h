#ifndef AUTHJAVASCRIPT_H
#define AUTHJAVASCRIPT_H

#include <QObject>
#include <functional>

#include "WrapperJavascript.h"

namespace auth {

class Auth;
struct LoginInfo;

class AuthJavascript: public WrapperJavascript {
    Q_OBJECT
public:
    explicit AuthJavascript(QObject *parent = nullptr);

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
    void sendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error);

public slots:

    void onSendLoginInfoResponseSig(const LoginInfo &response, const TypedException &error);

private:
    Auth *m_authManager;
};

}
#endif // AUTHJAVASCRIPT_H
