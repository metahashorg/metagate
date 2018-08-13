#ifndef MAKEJSFUNCPARAMETERS_H
#define MAKEJSFUNCPARAMETERS_H

#include <QString>
#include <QJsonDocument>

#include <string>

#include "TypedException.h"
#include "check.h"
#include "Log.h"

template<typename T>
struct Opt {
    T value;
    bool isSet = false;

    Opt() = default;

    explicit Opt(const T &value)
        : value(value)
        , isSet(true)
    {}

    Opt<T>& operator=(const T &val) {
        value = val;
        isSet = true;
        return *this;
    }

    const T& get() const {
        CHECK(isSet, "Not set");
        return value;
    }

    const T& getWithoutCheck() const {
        return value;
    }
};

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

inline QString toJsString(const int &arg) {
    return QString::fromStdString(std::to_string(arg));
}

inline QString toJsString(const long long int &arg) {
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
    static_assert(!std::is_same<typename std::decay<decltype(arg)>::type, char const*>::value, "const char* not allowed");
    return toJsString(arg);
}

template<typename Arg, typename... Args>
inline QString append(const Arg &arg, Args&& ...args) {
    return append(arg) + ", " + append(std::forward<Args>(args)...);
}

template<typename Arg>
inline QString appendOpt(bool withoutCheck, const Arg &argOpt) {
    const auto &arg = withoutCheck ? argOpt.getWithoutCheck() : argOpt.get();
    static_assert(!std::is_same<typename std::decay<decltype(arg)>::type, char const*>::value, "const char* not allowed");
    return toJsString(arg);
}

template<typename Arg, typename... Args>
inline QString appendOpt(bool withoutCheck, const Arg &arg, Args&& ...args) {
    return appendOpt(withoutCheck, arg) + ", " + appendOpt(withoutCheck, std::forward<Args>(args)...);
}

template<bool isLastArg, typename... Args>
inline QString makeJsFunc2(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args) {
    const bool withoutCheck = exception.numError != TypeErrors::NOT_ERROR;
    QString jScript = function + "(";
    jScript += appendOpt<Args...>(withoutCheck, std::forward<Args>(args)...) + ", ";
    jScript += append(exception.numError, exception.description);
    if (isLastArg) {
        jScript += ", " + toJsString(lastArg);
    }
    jScript += ");";
    return jScript;
}

template<class Function>
TypedException apiVrapper2(const Function &func) {
    try {
        func();
        return TypedException();
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        return e;
    } catch (const Exception &e) {
        LOG << "Error " << e;
        return TypedException(TypeErrors::OTHER_ERROR, e);
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        return TypedException(TypeErrors::OTHER_ERROR, e.what());
    } catch (...) {
        LOG << "Unknown error";
        return TypedException(TypeErrors::OTHER_ERROR, "Unknown error");
    }
}

#endif // MAKEJSFUNCPARAMETERS_H
