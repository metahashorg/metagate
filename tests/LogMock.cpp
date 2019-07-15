#include "Log.h"

#include <fstream>
#include <iostream>

#include <QString>

#include "check.h"

PeriodicLog::PeriodicLog() = default;

PeriodicLog::PeriodicLog(const std::string &str)
    : name(str)
{}

PeriodicLog PeriodicLog::make(const std::string &str) {
    CHECK(!str.empty(), "periodic name empty");
    return PeriodicLog(str);
}

bool PeriodicLog::notSet() const {
    return name.empty();
}

Log_::Log_(const std::string &/*fileName*/) {

}

bool Log_::processPeriodic(const std::string &/*s*/, std::string &/*addedStr*/, std::string &/*periodicStrFirstLine*/, std::string &/*periodicStrSecondLine*/) {
    return false;
}

void Log_::finalize() noexcept {
    try {
        const std::string &toCoutStr = ssCout.str();

        std::cout << toCoutStr << std::endl;
    } catch (...) {
        std::cerr << "Error";
    }
}

void Log_::print(const QString &s) {
    print(s.toStdString());
}

void Log_::print(const PeriodicLog &/*p*/) {

}

void Log_::print(const std::string &t) {
    if (t.empty()) {
        ssCout << "<empty>";
    } else {
        ssCout << t;
    }
}

void initLog() {
}

AddFileNameAlias_::AddFileNameAlias_(const std::string &/*fileName*/, const std::string &/*alias*/) {
}
