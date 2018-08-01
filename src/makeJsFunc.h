#ifndef MAKEJSFUNCPARAMETERS_H
#define MAKEJSFUNCPARAMETERS_H

#include <QString>
#include <QJsonDocument>

#include <string>

#include "TypedException.h"

inline QString toJsString(const QJsonDocument &arg) {
    QString json = arg.toJson(QJsonDocument::Compact);
    json.replace('\"', "\\\"");
    return "\"" + json + "\"";
}

inline QString toJsString(const QString &arg) {
    return "\"" + arg + "\"";
}

inline QString toJsString(const std::string &arg) {
    return "\"" + QString::fromStdString(arg) + "\"";
}

inline QString toJsString(const char *arg) {
    return "\"" + QString(arg) + "\"";
}

inline QString toJsString(const int &arg) {
    return QString::fromStdString(std::to_string(arg));
}

inline QString toJsString(bool arg) {
    if (arg) {
        return "true";
    } else {
        return "false";
    }
}

inline QString toJsString(const size_t &arg) {
    return QString::fromStdString(std::to_string(arg));
}

template<typename Arg>
inline QString append(const Arg &arg) {
    return toJsString(arg);
}

template<typename Arg, typename... Args>
inline QString append(const Arg &arg, Args&& ...args) {
    return toJsString(arg) + ", " + append(std::forward<Args>(args)...);
}

template<size_t index, typename... Args>
struct appendT0 { // TODO переписать на if constexpr, когда будет c++ 17
    QString operator()(const std::tuple<std::decay_t<Args>...> &args) {
        return appendT0<index - 1, Args...>()(args) + ", " + toJsString(std::get<index - 1>(args));
    }};

template<typename... Args>
struct appendT0<0, Args...> {
    QString operator()(const std::tuple<std::decay_t<Args>...> &args) {
        return "";
    }
};

template<typename... Args>
struct appendT0<1, Args...> {
    QString operator()(const std::tuple<std::decay_t<Args>...> &args) {
        return toJsString(std::get<0>(args));
    }
};

template<typename... Args>
inline QString appendT(const std::tuple<std::decay_t<Args>...> &args) {
    constexpr size_t tupleSize = std::tuple_size<std::tuple<std::decay_t<Args>...>>::value;
    return appendT0<tupleSize, Args...>()(args);
}

template<bool isLastArg, typename... Args>
struct JsFunc {

    QString function;

    QString lastArg;

    TypedException exception;

    std::tuple<std::decay_t<Args>...> args;

    JsFunc(const QString &function, const QString &lastArg, const TypedException &exception, Args&&... args)
        : function(function)
        , lastArg(lastArg)
        , exception(exception)
        , args(std::make_tuple(std::forward<Args>(args)...))
    {}

    JsFunc(const QString &function, const TypedException &exception)
        : function(function)
        , exception(exception)
    {}

    JsFunc() = default;

    template<typename T>
    void setFirstArgument(const T &t) {
        std::get<0>(args) = t;
    }

    QString invoke() const {
        QString jScript = function + "(";
        jScript += appendT<Args...>(args) + ", ";
        jScript += append(exception.numError, exception.description);
        if (isLastArg) {
            jScript += ", \"" + lastArg + "\"";
        }
        jScript += ");";
        return jScript;
    }

};

template<bool isLastArg, typename... Args>
inline JsFunc<isLastArg, Args...> makeJsFunc(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args) {
    return JsFunc<isLastArg, Args...>(function, lastArg, exception, std::forward<Args>(args)...);
}

#endif // MAKEJSFUNCPARAMETERS_H
