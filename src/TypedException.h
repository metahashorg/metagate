#ifndef TYPEDEXCEPTION_H
#define TYPEDEXCEPTION_H

#include <functional>

enum TypeErrors {
    NOT_ERROR = 0,
    DONT_CREATE_FOLDER = 1,
    DONT_CREATE_KEY = 2,
    INCORRECT_PASSWORD = 3,
    DONT_SIGN = 4,
    INCORRECT_USER_DATA = 5,
    INCORRECT_ADDRESS_OR_PUBLIC_KEY = 6,
    PRIVATE_KEY_ERROR = 7,

    QR_ENCODE_ERROR = 8,

    MESSENGER_SERVER_ERROR_ADDRESS_EXIST = 9,
    MESSENGER_SERVER_ERROR_SIGN_OR_ADDRESS_INVALID = 10,
    MESSENGER_SERVER_ERROR_INCORRECT_JSON = 11,
    MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND = 12,
    MESSENGER_SERVER_ERROR_CHANNEL_EXIST = 13,
    MESSENGER_SERVER_ERROR_CHANNEL_NOT_PERMISSION = 14,
    MESSENGER_SERVER_ERROR_CHANNEL_NOT_FOUND = 15,
    MESSENGER_SERVER_ERROR_OTHER = 16,

    WALLET_NOT_UNLOCK = 17,
    WALLET_OTHER = 18,
    INCOMPLETE_USER_INFO = 19,
    CHANNEL_TITLE_INCORRECT = 20,

    OTHER_ERROR = 1000
};

struct TypedException {
    TypeErrors numError;
    std::string description;

    TypedException()
        : numError(TypeErrors::NOT_ERROR)
        , description("")
    {}

    TypedException(const TypeErrors &numError, const std::string &description)
        : numError(numError)
        , description(description)
    {}

    bool isSet() const {
        return numError != TypeErrors::NOT_ERROR;
    }
};

#define throwErrTyped(type, s) { \
{ \
    throw ::TypedException(type, s + std::string(". Error at file ") \
        + std::string(__FILE__) + std::string(" line ") + std::to_string(__LINE__)); \
} \
}

#define CHECK_TYPED(v, type, s) { \
if (!(v)) { \
    throwErrTyped(type, s); \
} \
}

TypedException apiVrapper2(const std::function<void()> &func);

#endif // TYPEDEXCEPTION_H
