#include "MhPayEventHandler.h"

#include <QWidget>
#include <QFileOpenEvent>

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"

#include "RunGuard.h"

#include "mainwindow.h"
#include "PagesMappings.h"

SET_LOG_NAMESPACE("MW");

MhPayEventHandler::MhPayEventHandler(RunGuard &runGuard)
    : runGuard(runGuard)
{
    timer.setInterval(milliseconds(1s).count());
    CHECK(connect(&timer, &QTimer::timeout, this, &MhPayEventHandler::timerEvent), "not connect timerEvent");
    timer.start();
}

void MhPayEventHandler::setMainWindow(MainWindow *mw) {
    CHECK(mw != nullptr, "Incorrect mainwindow reference");
    mainWindow = mw;
    if (lastUrl != QUrl()) {
        emit mainWindow->processExternalUrl(lastUrl);
    }
}

bool MhPayEventHandler::processUrl(const QUrl &url) {
    if (url.toString().startsWith(METAHASH_PAY_URL)) {
        LOG << "Getted metahash handl url " << url.toString();
        if (mainWindow != nullptr) {
            emit mainWindow->processExternalUrl(url);
        } else {
            lastUrl = url;
        }
        return true;
    } else {
        return false;
    }
}

bool MhPayEventHandler::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = static_cast<QFileOpenEvent *>(event);
        const QUrl url = openEvent->url();
        if (processUrl(url)) {
            mainWindow->showNormal();
            return true;
        } else {
            return QObject::eventFilter(object, event);
        }
    } else {
        // standard event processing
        return QObject::eventFilter(object, event);
    }
}

void MhPayEventHandler::timerEvent() {
BEGIN_SLOT_WRAPPER
    mainWindow->setWindowFlags(mainWindow->windowFlags() & ~Qt::WindowStaysOnTopHint);
    //mainWindow->show();

    const std::string commandLine = runGuard.getValueAndReset();
    if (commandLine.empty()) {
        return;
    }
    const QUrl url(QString::fromStdString(commandLine));
    const bool success = processUrl(url);
    if (success) {
        if (mainWindow != nullptr) {
            mainWindow->setWindowFlags(mainWindow->windowFlags() | Qt::WindowStaysOnTopHint);
            mainWindow->showNormal();
            mainWindow->show();
            mainWindow->activateWindow();
        }
    }
END_SLOT_WRAPPER
}

void MhPayEventHandler::processCommandLine(const QString &arg) {
    const QUrl url(arg);
    processUrl(url);
}
