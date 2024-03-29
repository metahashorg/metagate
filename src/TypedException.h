#ifndef TYPEDEXCEPTION_H
#define TYPEDEXCEPTION_H

#include <functional>
#include <string>

struct Exception;

enum TypeErrors {
    NOT_ERROR = 0,
    DONT_CREATE_FOLDER = 1,
    DONT_CREATE_KEY = 2,
    INCORRECT_PASSWORD = 3,
    DONT_SIGN = 4,
    INCORRECT_USER_DATA = 5,
    INCORRECT_ADDRESS_OR_PUBLIC_KEY = 6,
    PRIVATE_KEY_ERROR = 7,
    INCORRECT_VALUE_OR_FEE = 9,

    QR_ENCODE_ERROR = 8,

    CLIENT_ERROR = 10,

    MESSENGER_SERVER_ERROR_ADDRESS_EXIST = 100,
    MESSENGER_SERVER_ERROR_SIGN_OR_ADDRESS_INVALID = 101,
    MESSENGER_SERVER_ERROR_INCORRECT_JSON = 102,
    MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND = 103,
    MESSENGER_SERVER_ERROR_CHANNEL_EXIST = 104,
    MESSENGER_SERVER_ERROR_CHANNEL_NOT_PERMISSION = 105,
    MESSENGER_SERVER_ERROR_CHANNEL_NOT_FOUND = 106,
    MESSENGER_SERVER_ERROR_OTHER = 107,

    WALLET_NOT_UNLOCK = 108,
    WALLET_OTHER = 109,
    INCOMPLETE_USER_INFO = 110,
    CHANNEL_TITLE_INCORRECT = 111,
    MESSENGER_NOT_CONFIGURED = 112,

    TRANSACTIONS_SERVER_SEND_ERROR = 200,
    TRANSACTIONS_SENDED_NOT_FOUND = 201,
    TRANSACTIONS_SERVER_NOT_FOUND = 202,

    INITIALIZER_TIMEOUT_ERROR = 400,

    EVENT_WATCHER_TIMEOUT = 500,

    PROXY_SERVER_ERROR = 600,
    PROXY_RESTART_ERROR = 601,

    EXTERCTORCONNECTOR_LOCALCONN_ERROR = 620,
    EXTERCTORCONNECTOR_URLCHANGED_ERROR = 621,

    OTHER_ERROR = 1000
};

struct TypedException {
    TypeErrors numError;
    std::string description;
    std::string file;

    TypedException()
        : numError(TypeErrors::NOT_ERROR)
        , description("")
    {}

    TypedException(const TypeErrors &numError, const std::string &description, const std::string &file="")
        : numError(numError)
        , description(description)
        , file(file)
    {}

    TypedException(const TypeErrors &numError, const Exception &exception);

    bool isSet() const {
        return numError != TypeErrors::NOT_ERROR;
    }
};

#define throwErrTyped(type, s) { \
{ \
    throw ::TypedException(type, s + std::string(". Error at file ") \
        + std::string(__FILE__) + std::string(" line ") + std::to_string(__LINE__), std::string(__FILE__)); \
} \
}

#define CHECK_TYPED(v, type, s) { \
if (!(v)) { \
    throwErrTyped(type, s); \
} \
}

TypedException apiVrapper2(const std::function<void()> &func);

TypedException apiVrapper2(const TypedException &exception, const std::function<void()> &func);

#endif // TYPEDEXCEPTION_H
