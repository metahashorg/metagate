#include "Log.h"

#include <QApplication>

#include <thread>
#include <mutex>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>

#include <QDateTime>
#include <QString>

#include "utils.h"
#include "Paths.h"
#include "check.h"

#include "duration.h"

class LogImplVars {
public:

    std::ofstream __log_file__;
    std::ofstream __log_file2__;

    std::mutex mutGlobal;

    struct PeriodicStruct {
        std::string content;
        std::vector<milliseconds> periods;
        time_point lastPeriod;
        int iteration = 0;
        size_t count = 20;

        milliseconds calcInterval(const time_point &now) const {
            return std::chrono::duration_cast<milliseconds>(now - lastPeriod);
        }
    };

    static std::string makeStrPeriodic(const LogImplVars::PeriodicStruct &p) {
        std::string addedStr = "Changed " + std::to_string(p.iteration) + ". ";
        addedStr += "Repeated " + std::to_string(p.periods.size()) + " times: ";
        addedStr = std::accumulate(p.periods.begin(), p.periods.end(), addedStr, [](const std::string &str, const milliseconds &ms) {
            return str + std::to_string(std::chrono::duration_cast<seconds>(ms).count()) + ", ";
        });
        addedStr += " seconds";
        return addedStr;
    }

    std::map<std::string, PeriodicStruct> periodics;
    std::mutex mutPeriodics;

    ~LogImplVars() {
        try {
            std::unique_lock<std::mutex> lock(mutPeriodics);
            const std::map<std::string, PeriodicStruct> copy = periodics;
            lock.unlock();
            for (const auto &pair: copy) {
                if (!pair.second.periods.empty()) {
                    const std::string toLog = makeStrPeriodic(pair.second);
                    std::lock_guard<std::mutex> lockLog(mutGlobal);
                    __log_file__ << pair.first << ": " << toLog << std::endl;
                    __log_file2__ << pair.first << ": " << toLog << std::endl;
                }
            }
        } catch (...) {
            std::cout << "Error while end periodic log" << std::endl;
            __log_file__ << "Error while end periodic log" << std::endl;
            __log_file2__ << "Error while end periodic log" << std::endl;
        }
    }

};

static LogImplVars vars;

static std::map<std::string, std::string>& getFileNamesImpl(bool isRead) {
    static const auto mainThreadId = std::this_thread::get_id();
    const auto currentThread = std::this_thread::get_id();
    static bool readAccessInAnotherThread = false;
    readAccessInAnotherThread = readAccessInAnotherThread || (isRead && (currentThread != mainThreadId));
    if (!isRead) {
        if (currentThread != mainThreadId) {
            std::cout << "Set log alias in another thread" << std::endl;
            exit(1);
        }
        if (readAccessInAnotherThread) {
            std::cout << "Logger already used" << std::endl;
            exit(1);
        }
    }
    static std::map<std::string, std::string> fileNames;
    return fileNames;
}

static const std::map<std::string, std::string>& getFileNamesC() {
    return getFileNamesImpl(true);
}

static std::map<std::string, std::string>& getFileNamesN() {
    return getFileNamesImpl(false);
}

static size_t& getMaxAliasSize() {
    static size_t aliasSize = 0;
    return aliasSize;
}

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

Log_::Log_(const std::string &fileName) {
    const QDateTime now = QDateTime::currentDateTime();
    const std::string time = now.toString("yyyy.MM.dd_hh:mm:ss").toStdString();
    const auto tId = std::this_thread::get_id();
    ssLog << std::hex << std::noshowbase << tId << std::dec << " " << time;

    const auto found = getFileNamesC().find(fileName);
    if (found != getFileNamesC().end()) {
        const std::string &alias = found->second;
        ssLog << " " << std::setfill(' ') << std::setw(getMaxAliasSize()) << alias;
    } else {
        ssLog << std::setfill(' ') << std::setw(getMaxAliasSize() + 1) << " ";
    }
}

bool Log_::processPeriodic(const std::string &s, std::string &addedStr) {
    if (periodic.notSet()) {
        return true;
    } else {
        const time_point now = ::now();
        bool to_return;
        std::lock_guard<std::mutex> lock(vars.mutPeriodics);
        auto found = vars.periodics.find(periodic.str);
        if (found != vars.periodics.end()) {
            LogImplVars::PeriodicStruct &p = found->second;
            if (p.content == s) {
                const milliseconds interval = p.calcInterval(now);
                p.periods.push_back(interval);
                p.lastPeriod = now;
                if (p.periods.size() >= p.count) {
                    addedStr = LogImplVars::makeStrPeriodic(p);
                    p.periods.clear();
                }
                to_return = false;
            } else {
                addedStr = LogImplVars::makeStrPeriodic(p);
                p.periods.clear();
                p.lastPeriod = now;
                p.content = s;
                p.iteration++;
                to_return = true;
            }
        } else {
            LogImplVars::PeriodicStruct p;
            p.content = s;
            p.lastPeriod = now;
            vars.periodics[periodic.str] = p;
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

        std::lock_guard<std::mutex> lock(vars.mutGlobal);
        std::cout << toCoutStr << *pManip;
        if (!addedStr.empty()) {
            vars.__log_file__ << toLogStr << addedStr << *pManip;
            vars.__log_file2__ << toLogStr << addedStr << *pManip;
        }
        if (isPrint) {
            vars.__log_file__ << toLogStr << toCoutStr << *pManip;
            vars.__log_file2__ << toLogStr << toCoutStr << *pManip;
        }
    } catch (...) {
        std::cerr << "Error";
    }
}

void Log_::print(const QString &s) {
    print(s.toStdString());
}

void Log_::print(const PeriodicLog &p) {
    CHECK(periodic.notSet(), "Periodic already set");
    CHECK(!p.notSet(), "Periodic not set");
    periodic = p;
    ssLog << ": \'" << p.str << "\'";
}

void Log_::print(const std::string &t) {
    if (t.empty()) {
        ssCout << "<empty>";
    } else {
        ssCout << t;
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

    vars.__log_file__.open(path, std::ios_base::trunc);
    vars.__log_file2__.open(path2, std::ios_base::trunc);
}

AddFileNameAlias_::AddFileNameAlias_(const std::string &fileName, const std::string &alias) {
    auto &aliases = getFileNamesN();
    aliases[fileName] = alias;

    size_t &aliasSize = getMaxAliasSize();
    aliasSize = std::max(aliasSize, alias.size());
}
