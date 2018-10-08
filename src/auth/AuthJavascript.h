#ifndef AUTHJAVASCRIPT_H
#define AUTHJAVASCRIPT_H

#include <QObject>

struct TypedException;

namespace auth
{

class AuthJavascript : public QObject
{
    Q_OBJECT
public:
    explicit AuthJavascript(QObject *parent = nullptr);

public slots:
    Q_INVOKABLE void login(QString username, QString password);

signals:

    void jsRunSig(QString jsString);

private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);
};

}
#endif // AUTHJAVASCRIPT_H
