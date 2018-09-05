#include <QTest>

#include <iostream>

#include "tst_rsa.h"
#include "tst_Bitcoin.h"
#include "tst_Ethereum.h"
#include "tst_Metahash.h"

int main(int argc, char *argv[]) {
    int status = 0;
    auto ASSERT_TEST = [&status, argc, argv](QObject* obj) {
        if (status) {
            return;
        }
        status |= QTest::qExec(obj, argc, argv);
        delete obj;
    };

    ASSERT_TEST(new tst_rsa());
    ASSERT_TEST(new tst_Bitcoin());
    ASSERT_TEST(new tst_Ethereum());
    ASSERT_TEST(new tst_Metahash());

    return status;
}
