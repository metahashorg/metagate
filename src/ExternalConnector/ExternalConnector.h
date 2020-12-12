#ifndef EXTERNALCONNECTOR_H
#define EXTERNALCONNECTOR_H

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/CallbackCallWrapper.h"

class MainWindow;

class ExternalConnector : public CallbackCallWrapper
{
    Q_OBJECT
public:
    using GetUrlCallback = CallbackWrapper<void(const QString &url)>;
    using SetUrlCallback = CallbackWrapper<void()>;

    explicit ExternalConnector(MainWindow &mainWindow, QObject *parent = nullptr);

signals:
    void urlChanged(const QString &url);

    void getUrl(const GetUrlCallback &callback);
    void setUrl(const QString &url, const SetUrlCallback &callback);

private slots:
    void onGetUrl(const GetUrlCallback &callback);
    void onSetUrl(const QString &url, const SetUrlCallback &callback);

private:
    MainWindow &mainWindow;
};

#endif // EXTERNALCONNECTOR_H
