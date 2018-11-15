#include "Log.h"

#include <QApplication>

#include <thread>
#include <mutex>
#include <fstream>
#include <iostream>
#include <map>

#include <QDateTime>
#include <QString>

#include "utils.h"
#include "Paths.h"
#include "check.h"

#include "duration.h"

static std::ofstream __log_file__;
static std::ofstream __log_file2__;

static std::mutex mutGlobal;

struct PeriodicStruct {
    std::string content;
    std::vector<milliseconds> periods;
    time_point lastPeriod;
    int iteration = 0;
    size_t count = 10;

    milliseconds calcInterval(const time_point &now) const {
        return std::chrono::duration_cast<milliseconds>(now - lastPeriod);
    }
};

static std::map<std::string, PeriodicStruct> periodics;
static std::mutex mutPeriodics;

PeriodicLog::PeriodicLog() = default;

PeriodicLog::PeriodicLog(const std::string &str)
    : str(str)
{}

PeriodicLog PeriodicLog::make(const std::string &str) {
    CHECK(!str.empty(), "periodic name empty");
    return PeriodicLog(str);
}

bool PeriodicLog::notSet() const {
    return str.empty();
}

Log_ &Log_::operator <<(const QString &s) {
    print(s.toStdString());
    return *this;
}

Log_ &Log_::operator <<(const PeriodicLog &p) {
    CHECK(periodic.notSet(), "Periodic already set");
    CHECK(!p.notSet(), "Periodic not set");
    periodic = p;
    ssLog << " \'" << p.str << "\'";
    return *this;
}

Log_::Log_() {
    const QDateTime now = QDateTime::currentDateTime();
    const std::string time = now.toString("yyyy.MM.dd_hh:mm:ss").toStdString();
    const auto tId = std::this_thread::get_id();
    ssLog << std::hex << tId << std::dec << " " << time;
}

bool Log_::processPeriodic(const std::string &s, std::string &addedStr) {
    if (periodic.notSet()) {
        return true;
    } else {
        const auto makeStr = [](const PeriodicStruct &p) {
            std::string addedStr = "Changed " + std::to_string(p.iteration) + ". ";
            addedStr += "Repeated " + std::to_string(p.periods.size()) + " times: ";
            addedStr = std::accumulate(p.periods.begin(), p.periods.end(), addedStr, [](const std::string &str, const milliseconds &ms) {
                return str + std::to_string(std::chrono::duration_cast<seconds>(ms).count()) + ", ";
            });
            addedStr += " seconds";
            return addedStr;
        };

        const time_point now = ::now();
        bool to_return;
        std::lock_guard<std::mutex> lock(mutPeriodics);
        auto found = periodics.find(periodic.str);
        if (found != periodics.end()) {
            PeriodicStruct &p = found->second;
            if (p.content == s) {
                const milliseconds interval = p.calcInterval(now);
                p.periods.push_back(interval);
                p.lastPeriod = now;
                if (p.periods.size() >= p.count) {
                    addedStr = makeStr(p);
                    p.periods.clear();
                }
                to_return = false;
            } else {
                addedStr = makeStr(p);
                p.periods.clear();
                p.lastPeriod = now;
                p.content = s;
                p.iteration++;
                to_return = true;
            }
        } else {
            PeriodicStruct p;
            p.content = s;
            p.lastPeriod = now;
            periodics[periodic.str] = p;
            to_return = true;
        }
        return to_return;
    }
}

void Log_::finalize(std::ostream &(*pManip)(std::ostream &)) noexcept {
    try {
        const std::string &toCoutStr = ssCout.str();
        const std::string &toLogStr = ssLog.str() + ": ";

        std::string addedStr;
        const bool isPrint = processPeriodic(toCoutStr, addedStr);

        std::lock_guard<std::mutex> lock(mutGlobal);
        std::cout << toCoutStr << *pManip;
        if (!addedStr.empty()) {
            __log_file__ << toLogStr << addedStr << *pManip;
            __log_file2__ << toLogStr << addedStr << *pManip;
        }
        if (isPrint) {
            __log_file__ << toLogStr << toCoutStr << *pManip;
            __log_file2__ << toLogStr << toCoutStr << *pManip;
        }
    } catch (...) {
        std::cerr << "Error";
    }
}

void initLog() {
    const QString logPath = getLogPath();

    const size_t MAX_LOG_FILES = 20;
    const QDate today = QDate::currentDate();
    std::vector<QFileInfo> files;
    const auto tmp = QDir(logPath).entryInfoList(QStringList("log.*.txt"), QDir::Files);
    std::copy(tmp.begin(), tmp.end(), std::back_inserter(files));
    std::sort(files.begin(), files.end(), [&today](const QFileInfo &f1, const QFileInfo &f2) {
        return f1.created().date().daysTo(today) < f2.created().date().daysTo(today);
    });
    files.erase(files.begin(), files.begin() + std::min(MAX_LOG_FILES, files.size()));

    for (const QFileInfo &fileInfo: files) {
        const QString filepath = fileInfo.absoluteFilePath();
        removeFile(filepath);
    }

    const system_time_point now = ::system_now();
    const QString logFile = QString::fromStdString("log." + std::to_string(systemTimePointToInt(now)) + ".txt");
    const QString logFile2 = makePath(QApplication::applicationDirPath(), "log.txt");

    const QString fullLogPath = makePath(logPath, logFile);
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
