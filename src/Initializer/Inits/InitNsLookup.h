#ifndef INIT_NSLOOKUP_H
#define INIT_NSLOOKUP_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>

class NsLookup;
struct TypedException;

namespace initializer {

class InitializerJavascript;

class InitNsLookup: public QObject, public InitInterface {
    Q_OBJECT
public:

    using Return = NsLookup*;

public:

    InitNsLookup(QThread *mainThread, Initializer &manager);

    ~InitNsLookup() override;

    void complete() override;

    Return initialize();

    static int countEvents() {
        return 2;
    }

private:

    void sendInitSuccess(const TypedException &exception);

private slots:

    void onServersFlushed(const TypedException &exception);

private:

    std::unique_ptr<NsLookup> nsLookup;

    bool isFlushed = false;

};

}

#endif // INIT_NSLOOKUP_H
