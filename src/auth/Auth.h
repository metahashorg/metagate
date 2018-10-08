#ifndef AUTH_H
#define AUTH_H

#include <QObject>
#include "HttpClient.h"

namespace auth
{
class AuthJavascript;

class Auth : public QObject
{
public:
    explicit Auth(AuthJavascript &javascriptWrapper, QObject *parent = nullptr);

    QString makeLoginRequest(QString login, QString password);

private:
    AuthJavascript &javascriptWrapper;
    HttpSimpleClient tcpClient;
};

}

#endif // AUTH_H
