#include "Log.h"

#include <QApplication>
#include <QDir>

#include "utils.h"

std::ofstream __log_file__;

void initLog() {
#ifdef TARGET_WINDOWS
    auto path = makePath(QApplication::applicationDirPath(), "log.txt").toStdWString();
#else
    auto path = makePath(QApplication::applicationDirPath(), "log.txt").toStdString();
#endif
    __log_file__.open(path, std::ios_base::trunc);
}
