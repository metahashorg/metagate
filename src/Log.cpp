#include "Log.h"

#include <QApplication>

#include <thread>
#include <mutex>
#include <fstream>
#include <iostream>

#include <QDateTime>
#include <QString>

#include "utils.h"
#include "Paths.h"

#include "duration.h"

static std::ofstream __log_file__;
static std::ofstream __log_file2__;

static std::mutex mutGlobal;

Log_ &Log_::operator <<(const QString &s) {
    print(s.toStdString());
    return *this;
}

Log_::Log_() {
    const QDateTime now = QDateTime::currentDateTime();
    const std::string time = now.toString("yyyy.MM.dd_hh:mm:ss").toStdString();
    const auto tId = std::this_thread::get_id();
    ssLog << std::hex << tId << std::dec << " " << time << " ";
}

void Log_::finalize(std::ostream &(*pManip)(std::ostream &)) noexcept {
    try {
        const std::string &toCoutStr = ssCout.str();
        const std::string &toLogStr = ssLog.str();

        std::lock_guard<std::mutex> lock(mutGlobal);
        std::cout << toCoutStr << *pManip;
        __log_file__ << toLogStr << toCoutStr << *pManip;
        __log_file2__ << toLogStr << toCoutStr << *pManip;
    } catch (...) {
        std::cerr << "Error";
    }
}

void initLog() {
    const system_time_point now = ::system_now();
    const QString logFile = QString::fromStdString("log." + std::to_string(systemTimePointToInt(now)) + ".txt");
    const QString logFile2 = makePath(QApplication::applicationDirPath(), "log.txt");

    const QString fullLogPath = makePath(getLogPath(), logFile);
#ifdef TARGET_WINDOWS
    auto path = fullLogPath.toStdWString();
    auto path2 = logFile2.toStdWString();
#else
    auto path = fullLogPath.toStdString();
    auto path2 = logFile2.toStdString();
#endif

    __log_file__.open(path, std::ios_base::trunc);
    __log_file2__.open(path2, std::ios_base::trunc);
}
