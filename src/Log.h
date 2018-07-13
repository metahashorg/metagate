#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <iostream>
#include <ctime>

#include <QString>

#include "duration.h"

extern std::ofstream __log_file__;
extern std::ofstream __log_file2__;

struct Log_ {

    bool isSetTimestamp = true;

    Log_() = default;

    template<typename T>
    Log_& operator <<(T t) {
        print(t);
        return *this;
    }

    Log_& operator <<(const QString &s) {
        print(s.toStdString());
        return *this;
    }

    void finalize(std::ostream&(*pManip)(std::ostream&)) {
        std::cout << *pManip;
        __log_file__ << *pManip;
        __log_file2__ << *pManip;
    }

    ~Log_() {
        finalize(std::endl);
    }

private:

    template<typename T>
    void print(T t) {
        std::cout << t;
        if (isSetTimestamp) {
            const auto p = std::chrono::system_clock::now();
            const std::time_t t = std::chrono::system_clock::to_time_t(p);
            std::string cTime = std::ctime(&t);
            if (cTime[cTime.size() - 1] == '\n') {
                cTime = cTime.substr(0, cTime.size() - 1);
            }
            __log_file__ << cTime << " ";
            __log_file2__ << cTime << " ";
        }
        __log_file__ << t;
        __log_file2__ << t;
        isSetTimestamp = false;
    }

};

void initLog();

#define LOG Log_()

#endif // LOG_H
