#include "TorProxy.h"

#include <QProcess>
#include <QDir>
#include <QApplication>
#include <QTextStream>

#include "Log.h"
#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("TOR");

namespace tor {

#ifdef Q_OS_WIN
const QString TorExec = QLatin1String("tor.exe");
#else
const QString TorExec = QLatin1String("tor");
#endif

const QString TorConfig = QLatin1String("torrc");

static QDir getTorDir()
{
    QDir dir(qApp->applicationDirPath());
    dir.cd(QStringLiteral("tor"));
/*#ifdef Q_OS_MACOS
    dir.cdUp();
    dir.cdUp();
    dir.cdUp();
    dir.cd(MAINTENANCETOOL + QStringLiteral(".app"));
    dir.cd(QStringLiteral("Contents"));
    dir.cd(QStringLiteral("MacOS"));
#endif*/
    return dir;
}

TorProxy::TorProxy(QObject *parent)
    : QObject(parent)
    , torProc(new QProcess(this))
{
    torProc->setReadChannel(QProcess::StandardOutput);

    connect(torProc, &QProcess::readyReadStandardOutput, [this](){
        QTextStream stream(torProc);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            LOG << "OUT: " << line;
        }
        //QString perr = p.readAllStandardError();
    });

    connect(torProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [](int exitCode, QProcess::ExitStatus exitStatus){
        LOG << "Tor finished " << exitCode << exitStatus;
    });


    LOG << "Tor working dir " << getTorDir().absolutePath();
    torProc->setWorkingDirectory(getTorDir().absolutePath());
}

void TorProxy::start()
{
    const QDir tordir = getTorDir();
    const QStringList args{QStringLiteral("-f"),
        tordir.filePath(TorConfig)};
    torProc->start(tordir.filePath(TorExec), args);
//    CHECK(checkPrc.waitForStarted(), std::string("Process waitForStarted error ") + std::to_string(checkPrc.error()));
//    CHECK(checkPrc.waitForFinished(), std::string("Process waitForFinished error ") + std::to_string(checkPrc.error()));

//    QByteArray errStr = checkPrc.readAllStandardError();
//    LOG << "res " << errStr.toStdString();

//    QByteArray result = checkPrc.readAll();

}

}
