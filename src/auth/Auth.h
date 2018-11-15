#ifndef AUTH_H
#define AUTH_H

#include "TimerClass.h"
#include "HttpClient.h"
#include "client.h"

namespace auth
{
class AuthJavascript;
struct LoginInfo
{
    QString login = QString();
    QString token = QString();
    QString refresh = QString();
    bool isAuth = false;
    bool isTest = false;
    seconds expire;
    time_point saveTime;

    void clear()
    {
        login = QString();
        token = QString();
        isAuth = false;
        isTest = false;
    }
};

class Auth : public TimerClass
{
    Q_OBJECT
public:
    using Callback = std::function<void()>;

public:

    explicit Auth(AuthJavascript &javascriptWrapper, QObject *parent = nullptr);

signals:
    void logined(const QString &login);

    void logouted();

signals:
    void login(const QString &login, const QString &password);

    void logout();

    void check();

    void forceRefresh();

    void reEmit();

public slots:

    void onLogin(const QString &login, const QString &password);

    void onLogout();

    void onCheck();

    void onForceRefresh();

    void onReEmit();

private slots:

    void onCallbackCall(Callback callback);

    void onStarted();

    void onTimerEvent();

private:

    void logoutImpl();

    void readLoginInfo();

    void writeLoginInfo();

    void checkToken();

    void forceRefreshInternal();

    template<typename Func>
    void runCallback(const Func &callback);

    QString makeLoginRequest(const QString &login, const QString &password) const;

    QString makeCheckTokenRequest(const QString &token) const;

    QString makeRefreshTokenRequest(const QString &token) const;

    LoginInfo parseLoginResponse(const QString &response) const;

    bool parseCheckTokenResponse(const QString &response) const;

    LoginInfo parseRefreshTokenResponse(const QString &response) const;

private:
    AuthJavascript &javascriptWrapper;
    //HttpSimpleClient tcpClient;
    SimpleClient tcpClient;

    QString authUrl;
    QString hardwareId;
    LoginInfo info;

    seconds timeout;
};

}

#endif // AUTH_H
