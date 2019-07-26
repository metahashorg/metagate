#ifndef INIT_NSLOOKUP_H
#define INIT_NSLOOKUP_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>

class NsLookup;
class InfrastructureNsLookup;

struct TypedException;

namespace initializer {

class InitializerJavascript;

class InitNsLookup: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<NsLookup*, InfrastructureNsLookup*>;

public:

    InitNsLookup(QThread *mainThread, Initializer &manager);

    ~InitNsLookup() override;

    void completeImpl() override;

    Return initialize();

    static int countEvents() {
        return 2;
    }

    static int countCriticalEvents() {
        return 1;
    }

    static QString stateName();

signals:

    void serversFlushed(const TypedException &exception);

private:

    void sendInitSuccess(const TypedException &exception);

private slots:

    void onServersFlushed(const TypedException &exception);

private:

    std::unique_ptr<NsLookup> nsLookup;
    std::unique_ptr<InfrastructureNsLookup> infrastructureNsLookup;

};

}

#endif // INIT_NSLOOKUP_H
