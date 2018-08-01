#ifndef MAKEJSFUNCPARAMETERS_H
#define MAKEJSFUNCPARAMETERS_H

#include <QString>
#include <QJsonDocument>

#include <string>

static QString toJsString(const QJsonDocument &arg) {
    QString json = arg.toJson(QJsonDocument::Compact);
    json.replace('\"', "\\\"");
    return "\"" + json + "\"";
}

static QString toJsString(const QString &arg) {
    return "\"" + arg + "\"";
}

static QString toJsString(const std::string &arg) {
    return "\"" + QString::fromStdString(arg) + "\"";
}

static QString toJsString(const char *arg) {
    return "\"" + QString(arg) + "\"";
}

static QString toJsString(const int &arg) {
    return QString::fromStdString(std::to_string(arg));
}

static QString toJsString(bool arg) {
    if (arg) {
        return "true";
    } else {
        return "false";
    }
}

static QString toJsString(const size_t &arg) {
    return QString::fromStdString(std::to_string(arg));
}

template<typename Arg>
static QString append(const Arg &arg) {
    return toJsString(arg);
}

template<typename Arg, typename... Args>
static QString append(const Arg &arg, Args&& ...args) {
    return toJsString(arg) + ", " + append(std::forward<Args>(args)...);
}

template<typename... Args>
QString makeJsFunc(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args) {
    QString jScript = function + "(";
    jScript += append(std::forward<Args>(args)...) + ", ";
    jScript += append(exception.numError, exception.description);
    if (!lastArg.isNull() && !lastArg.isEmpty()) {
        jScript += ", \"" + lastArg + "\"";
    }
    jScript += ");";
    //LOG << "JScript " << jScript;
    return jScript;
}

#endif // MAKEJSFUNCPARAMETERS_H
