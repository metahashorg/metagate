#ifndef TYPEDEXCEPTION_H
#define TYPEDEXCEPTION_H

enum TypeErrors {
    NOT_ERROR = 0,
    DONT_CREATE_FOLDER = 1,
    DONT_CREATE_PUBLIC_KEY = 2,
    DONT_LOAD_PRIVATE_KEY = 3,
    DONT_SIGN = 4,
    OTHER_ERROR = 1000
};

struct TypedException {
    TypeErrors numError;
    std::string description;

    TypedException(const TypeErrors &numError, const std::string &description)
        : numError(numError)
        , description(description)
    {}
};

#endif // TYPEDEXCEPTION_H
