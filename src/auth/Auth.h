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
    bool isAuth = false;
    bool isTest = false;

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
    void logined();

    void logouted();

signals:
    void login(const QString &login, const QString &password);

    void logout();

    void check();

public slots:

    void onLogin(const QString &login, const QString &password);

    void onLogout();

    void onCheck();

private slots:

    void onCallbackCall(Callback callback);

    void onStarted();

    void onTimerEvent();

private:

    void doLogout();

    void readLoginInfo();

    void writeLoginInfo();

    void checkToken();

    template<typename Func>
    void runCallback(const Func &callback);

    QString makeLoginRequest(const QString &login, const QString &password);

    QString makeCheckTokenRequest(const QString &token);

    LoginInfo parseLoginResponse(const QString &response);

    bool parseCheckTokenResponse(const QString &response);

private:
    AuthJavascript &javascriptWrapper;
    //HttpSimpleClient tcpClient;
    SimpleClient tcpClient;

    QString authUrl;
    QString hardwareId;
    LoginInfo info;
};

}

#endif // AUTH_H
