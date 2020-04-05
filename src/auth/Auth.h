#ifndef AUTH_H
#define AUTH_H

#include "Network/SimpleClient.h"

#include "qt_utilites/TimerClass.h"
#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

struct TypedException;

namespace auth
{
class AuthJavascript;
enum AuthResult {
    OK = 0, REPEAT_REQUEST = 1, USER_NOT_FOUND = 2, NET_ERROR = 3
};

struct LoginInfo
{
    enum Type {
        LOGIN = 0, PATNERID = 1
    };

    QString login = QString();
    Type type = LOGIN;
    AuthResult error = OK;
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

    static QString getParnerId();

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

    void partnerIdLogin();

    void logout();

    void check();

    void forceRefresh();

    void reEmit();

public slots:

    void onLogin(const QString &login, const QString &password);

    void onPartnerIdLogin();

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

    void partnerIdLoginInternal(const QString &token);

    template<typename Func>
    void runCallback(const Func &callback);

    QString makeLoginRequest(const QString &login, const QString &password) const;

    QString makePartnerIdLoginRequest(const QString &token) const;

    QString makeCheckTokenRequest(const QString &token) const;

    QString makeRefreshTokenRequest(const QString &token) const;

    LoginInfo parseLoginResponse(const QString &response, const QString &login) const;

    LoginInfo parsePartnerIdLoginResponse(const QString &response) const;

    AuthResult parsePartnerIdLoginErrorResponse(const QString &response) const;

    bool parseCheckTokenResponse(const QString &response) const;

    LoginInfo parseRefreshTokenResponse(const QString &response, const QString &login, bool isTest, LoginInfo::Type type) const;

private:
    AuthJavascript &javascriptWrapper;
    SimpleClient tcpClient;

    QString authUrl;
    QString hardwareId;
    LoginInfo info;

    seconds timeout;

    int guardRefresh = 0;

    bool isInitialize = false;
};

}

#endif // AUTH_H
