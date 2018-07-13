#include "Log.h"

#include <QApplication>

#include "utils.h"
#include "Paths.h"

#include "duration.h"

std::ofstream __log_file__;
std::ofstream __log_file2__;

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
