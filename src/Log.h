#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <iostream>
#include <thread>

#include <QDateTime>
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
            const QDateTime now = QDateTime::currentDateTime();
            const std::string time = now.toString("yyyy.MM.dd_hh:mm:ss").toStdString();
            const auto tId = std::this_thread::get_id();
            __log_file__ << std::hex << tId << std::dec << " " << time << " ";
            __log_file2__ << std::hex << tId << std::dec << " " << time << " ";
        }
        __log_file__ << t;
        __log_file2__ << t;
        isSetTimestamp = false;
    }

};

void initLog();

#define LOG Log_()

#endif // LOG_H
