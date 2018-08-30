#ifndef LOG_H
#define LOG_H

#include <sstream>

class QString;

struct Log_ {

    Log_();

    template<typename T>
    Log_& operator <<(T t) {
        print(t);
        return *this;
    }

    Log_& operator <<(const QString &s);

    void finalize(std::ostream&(*pManip)(std::ostream&)) noexcept;

    ~Log_() noexcept {
        finalize(std::endl);
    }

private:

    template<typename T>
    void print(T t) {
        ssCout << t;
    }

    std::stringstream ssCout;
    std::stringstream ssLog;

};

void initLog();

#define LOG Log_()

#endif // LOG_H
