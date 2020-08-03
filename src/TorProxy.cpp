#include "TorProxy.h"

#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QApplication>
#include <QTextStream>

#include "Paths.h"
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

const QString TorDir = QLatin1String("tor");

const quint16 PortDefault = 34123;


static QDir getTorDir()
{
    QDir dir(qApp->applicationDirPath());
    dir.cd(TorDir);
    return dir;
}

static QString getTorExecPath()
{
    const QDir dir = getTorDir();
    return dir.filePath(TorExec);
}

TorProxy::TorProxy(QObject *parent)
    : QObject(parent)
    , torProc(new QProcess(this))
    , port(PortDefault)
{
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    port = settings.value("tor/port", PortDefault).toInt();

    torProc->setReadChannel(QProcess::StandardOutput);

    connect(torProc, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        LOG << "TOR start error " << error;
    });

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


}

void TorProxy::start()
{
    LOG << "Tor working dir " << getTorDir().absolutePath();
    torProc->setWorkingDirectory(getTorDir().absolutePath());

#ifdef Q_OS_LINUX
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LD_LIBRARY_PATH", getTorDir().absolutePath());
    torProc->setProcessEnvironment(env);
#endif

    const QString torconfig = getTorConfigPath();
    saveConfig(torconfig, getTorDataPath());

    const QStringList args{QStringLiteral("-f"), torconfig};
    torProc->start(getTorExecPath(), args);
//    CHECK(checkPrc.waitForStarted(), std::string("Process waitForStarted error ") + std::to_string(checkPrc.error()));
//    CHECK(checkPrc.waitForFinished(), std::string("Process waitForFinished error ") + std::to_string(checkPrc.error()));

//    QByteArray errStr = checkPrc.readAllStandardError();
//    LOG << "res " << errStr.toStdString();

//    QByteArray result = checkPrc.readAll();
    emit torProxyStarted(port);
}

void TorProxy::saveConfig(const QString &fn, const QString &datadir)
{
    QFile file(fn);
    CHECK(file.open(QIODevice::WriteOnly | QIODevice::Text), "Save torrc error");

    QTextStream out(&file);
    out << "SocksPort " << port << endl;
    out << "DataDirectory " << QDir::toNativeSeparators(datadir) << endl;
    file.close();
}

}
