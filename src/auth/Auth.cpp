#include "Auth.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

namespace auth
{

Auth::Auth(AuthJavascript &javascriptWrapper, QObject *parent)
    : QObject(parent)
    , javascriptWrapper(javascriptWrapper)
{

}

QString Auth::makeLoginRequest(QString login, QString password)
{
    QJsonObject request;
    request.insert("id", "1");
    request.insert("version", "1.0.0");
    request.insert("method", "user.auth");
    request.insert("uid", "machineUid");
    QJsonObject params;
    params.insert("login", login);
    params.insert("password", password);
    request.insert("params", params);
    return QString(QJsonDocument(request).toJson(QJsonDocument::Compact));
}
//{
//    "id": requestId,
//    "version": "1.0.0",
//    "method": "user.token",
//    "token": token,
//    "uid": machineUid,
//    "params": []
//}
}
