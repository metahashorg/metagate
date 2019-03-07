#ifndef MHPAYURLSCHEMEHANDLER_H
#define MHPAYURLSCHEMEHANDLER_H

#include <QObject>
#include <QWebEngineUrlSchemeHandler>

class QWebEngineUrlRequestJob;

class MHPayUrlSchemeHandler : public QWebEngineUrlSchemeHandler {
public:
    explicit MHPayUrlSchemeHandler(QObject *parent = nullptr);

    void requestStarted(QWebEngineUrlRequestJob *job) override;

};

#endif // MHPAYURLSCHEMEHANDLER_H
