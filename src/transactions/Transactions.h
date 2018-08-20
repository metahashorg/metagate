#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <QObject>

namespace transactions {

class Transactions : public QObject
{
    Q_OBJECT
public:
    explicit Transactions(QObject *parent = nullptr);

signals:

public slots:
};

}

#endif // TRANSACTIONS_H
