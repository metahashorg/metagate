#ifndef CONNECTIONRUNNABLE_H
#define CONNECTIONRUNNABLE_H

#include <QRunnable>

namespace proxy
{

class ConnectionRunnable : public QRunnable
{
public:
    ConnectionRunnable(qintptr socketDescriptor);

protected:
    void run();

public:
    qintptr m_socketDescriptor;
};

}

#endif // CONNECTIONRUNNABLE_H
