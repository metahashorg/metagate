#include "Log.h"

#include <QApplication>
#include <QDir>

std::ofstream __log_file__;

void initLog() {
#ifdef TARGET_WINDOWS
    auto path = QDir(QApplication::applicationDirPath()).filePath("log.txt").toStdWString();
#else
    auto path = QDir(QApplication::applicationDirPath()).filePath("log.txt").toStdString();
#endif
    __log_file__.open(path, std::ios_base::trunc);
}
