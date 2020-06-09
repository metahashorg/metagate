#ifndef AUTH_H
#define AUTH_H

#include "qt_utilites/TimerClass.h"
#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

class SimpleClient;
struct TypedException;

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
    system_time_point saveTimeSystem;
    time_point prevCheck;

    void clear()
    {
        login = QString();
        token = QString();
        isAuth = false;
        isTest = false;
    }
};

class Auth : public ManagerWrapper, public TimerClass
{
    Q_OBJECT
public:
    using LoginInfoCallback = CallbackWrapper<void(const LoginInfo &info)>;
public:

    explicit Auth(AuthJavascript &javascriptWrapper, QObject *parent = nullptr);

    ~Auth() override;

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

signals:

    void logined(bool isInit, const QString &login);

    void logined2(bool isInit, const QString &login, const QString &token);

    void checkTokenFinished(const TypedException &error);

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

signals:

    void getLoginInfo(const LoginInfoCallback &callback);

public slots:

    void onGetLoginInfo(const LoginInfoCallback &callback);

private:

    void logoutImpl();

    void readLoginInfo();

    void writeLoginInfo();

    // return true если проверка прошла и асинхронно она не будет выполняться
    bool checkToken();

    void forceRefreshInternal();

    template<typename Func>
    void runCallback(const Func &callback);

    QByteArray makeLoginRequest(const QString &login, const QString &password) const;

    QByteArray makeCheckTokenRequest(const QString &token) const;

    QByteArray makeRefreshTokenRequest(const QString &token) const;

    LoginInfo parseLoginResponse(const QByteArray &response, const QString &login) const;

    bool parseCheckTokenResponse(const QByteArray &response) const;

    LoginInfo parseRefreshTokenResponse(const QByteArray &response, const QString &login, bool isTest) const;

private:
    AuthJavascript &javascriptWrapper;
    SimpleClient *tcpClient;

    QString authUrl;
    QString hardwareId;
    LoginInfo info;

    seconds timeout;

    int guardRefresh = 0;

    bool isInitialize = false;
};

}

#endif // AUTH_H
