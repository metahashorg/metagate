#ifndef INIT_NSLOOKUP_H
#define INIT_NSLOOKUP_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>

class NsLookup;
class TypedException;

namespace initializer {

class InitializerJavascript;

class InitNsLookup: public QObject, public InitInterface {
    Q_OBJECT
public:

    using Return = std::reference_wrapper<NsLookup>;

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

    void sendFlushSuccess();

private slots:

    void onServersFlushed();

private:

    std::unique_ptr<NsLookup> nsLookup;

    bool isFlushed = false;

};

}

#endif // INIT_NSLOOKUP_H
