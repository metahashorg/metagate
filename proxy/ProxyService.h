#ifndef PROXYSERVICE_H
#define PROXYSERVICE_H

#include <QtService>

namespace proxy
{
class ProxyServer;

class ProxyService : public QtService<QCoreApplication>
{
public:
    ProxyService(int argc, char **argv);

protected:
    void start();
    void pause();
    void resume();

private:
    ProxyServer *daemon;
};

}

#endif // PROXYSERVICE_H
