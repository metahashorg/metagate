#include "Log.h"

#include <QApplication>

std::ofstream __log_file__;

void initLog() {
    __log_file__.open((QApplication::applicationDirPath() + "/log.txt").toStdString(), std::ios_base::trunc);
}
