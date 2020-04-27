#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEngineView>

class WebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    WebPage(QObject *parent);

    ~WebPage() override;
protected:

    virtual QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type) override;
};

class WebView : public QWebEngineView
{
    Q_OBJECT
public:
    WebView(QWidget *parent = nullptr);

protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

};

#endif // WEBVIEW_H
