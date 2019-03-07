#include "MhPayEventHandler.h"

#include <QWidget>
#include <QFileOpenEvent>

#include "check.h"
#include "Log.h"

#include "mainwindow.h"
#include "PagesMappings.h"

SET_LOG_NAMESPACE("MW");

void MhPayEventHandler::setMainWindow(MainWindow *mw) {
    CHECK(mw != nullptr, "Incorrect mainwindow reference");
    mainWindow = mw;
    if (lastUrl != QUrl()) {
        emit mainWindow->processExternalUrl(lastUrl);
    }
}

bool MhPayEventHandler::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::FileOpen) {
        auto *openEvent = static_cast<QFileOpenEvent *>(event);
        const QUrl url = openEvent->url();
        if (url.toString().startsWith(METAHASH_PAY_URL)) {
            LOG << "Getted metahash handl url " << url.toString();
            if (mainWindow != nullptr) {
                emit mainWindow->processExternalUrl(url);
            } else {
                lastUrl = url;
            }
            return true;
        } else {
            return QObject::eventFilter(object, event);
        }
    } else {
        // standard event processing
        return QObject::eventFilter(object, event);
    }
}
