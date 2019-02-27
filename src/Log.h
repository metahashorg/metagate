#ifndef LOG_H
#define LOG_H

#include <sstream>
#include <string>

class QString;

class PeriodicLog {
    friend struct Log_;
public:

    static PeriodicLog make(const std::string &str);

private:

    PeriodicLog();

    PeriodicLog(const std::string &str);

    bool notSet() const;

private:

    std::string str;
};

struct Log_ {

    Log_(const std::string &fileName);

    template<typename T>
    Log_& operator <<(T t) {
        print(t);
        return *this;
    }

    Log_& operator <<(const QString &s);

    Log_& operator <<(const PeriodicLog &p);

    void finalize(std::ostream&(*pManip)(std::ostream&)) noexcept;

    ~Log_() noexcept {
        finalize(std::endl);
    }

private:

    template<typename T>
    void print(T t) {
        ssCout << t;
    }

    bool processPeriodic(const std::string &s, std::string &addedStr);

    std::stringstream ssCout;
    std::stringstream ssLog;

    PeriodicLog periodic;

};

void initLog();

struct AddFileNameAlias_ {

    AddFileNameAlias_(const std::string &fileName, const std::string &alias);

};

#define LOG Log_(std::string(__FILE__))

#define SET_LOG_NAMESPACE(name) \
    static AddFileNameAlias_ log_alias_(std::string(__FILE__), name);

#endif // LOG_H
