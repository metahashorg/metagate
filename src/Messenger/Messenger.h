#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObject>

class Messenger : public QObject
{
    Q_OBJECT
public:
    explicit Messenger(QObject *parent = nullptr);

signals:

public slots:
};

#endif // MESSENGER_H