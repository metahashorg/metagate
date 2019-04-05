#include "tst_walletnamesdbstorage.h"

#include <QTest>

#include <iostream>

#include "check.h"

#include "WalletNamesDbStorage.h"
#include "WalletNamesDbRes.h"

using namespace wallet_names;

tst_WalletNamesDBStorage::tst_WalletNamesDBStorage(QObject *parent)
    : QObject(parent)
{
}

void tst_WalletNamesDBStorage::testGiveName() {
    if (QFile::exists(databaseFileName)) {
        QFile::remove(databaseFileName);
    }
    WalletNamesDbStorage db;
    db.init();

    db.giveNameWallet("123", "name1");
    db.giveNameWallet("234", "name2");
    db.giveNameWallet("345", "name1");
    db.giveNameWallet("234", "name3");

    std::vector<WalletInfo> wallets = db.getAllWallets();
    std::sort(wallets.begin(), wallets.end(), [](const WalletInfo &first, const WalletInfo &second) {
        return first.address < second.address;
    });

    QCOMPARE(wallets.size(), 3);
    QCOMPARE(wallets[0].address, "123");
    QCOMPARE(wallets[0].name, "name1");
    QCOMPARE(wallets[1].address, "234");
    QCOMPARE(wallets[1].name, "name3");
    QCOMPARE(wallets[2].address, "345");
    QCOMPARE(wallets[2].name, "name1");
}

static void checkInfo(const WalletInfo &info, const WalletInfo &second) {
    QCOMPARE(info.address, second.address);
    QCOMPARE(info.name, second.name);

    WalletInfo copy = info;
    std::sort(copy.infos.begin(), copy.infos.end(), [](const WalletInfo::Info &first, const WalletInfo::Info &second) {
        return std::make_tuple(first.user, first.currency, first.device) < std::make_tuple(second.user, second.currency, second.device);
    });

    WalletInfo secondCopy = second;
    std::sort(secondCopy.infos.begin(), secondCopy.infos.end(), [](const WalletInfo::Info &first, const WalletInfo::Info &second) {
        return std::make_tuple(first.user, first.currency, first.device) < std::make_tuple(second.user, second.currency, second.device);
    });

    QCOMPARE(secondCopy.infos.size(), copy.infos.size());
    for (size_t i = 0; i < copy.infos.size(); i++) {
        const auto &i1 = secondCopy.infos[i];
        const auto &i2 = copy.infos[i];
        QCOMPARE(i1.user, i2.user);
        QCOMPARE(i1.device, i2.device);
        QCOMPARE(i1.currency, i2.currency);
    }
}

void tst_WalletNamesDBStorage::testUpdateInfo() {
    if (QFile::exists(databaseFileName)) {
        QFile::remove(databaseFileName);
    }
    WalletNamesDbStorage db;
    db.init();

    db.giveNameWallet("123", "name1");
    WalletInfo wallet;
    wallet.address = "123";
    wallet.name = "name1";
    wallet.infos.emplace_back("u1", "d1", "c1");
    wallet.infos.emplace_back("u2", "d2", "c2");
    wallet.infos.emplace_back("u3", "d3", "c3");
    db.updateWalletInfo(wallet.address, wallet.infos);

    db.giveNameWallet("345", "name2");
    WalletInfo wallet2;
    wallet2.address = "345";
    wallet2.name = "name2";
    wallet2.infos.emplace_back("u4", "d4", "c4");
    wallet2.infos.emplace_back("u5", "d5", "c5");
    db.updateWalletInfo(wallet2.address, wallet2.infos);

    std::vector<WalletInfo> wallets = db.getAllWallets();
    std::sort(wallets.begin(), wallets.end(), [](const WalletInfo &first, const WalletInfo &second) {
        return first.address < second.address;
    });

    QCOMPARE(wallets.size(), 2);

    checkInfo(wallets[0], wallet);
    checkInfo(wallets[1], wallet2);
}

void tst_WalletNamesDBStorage::testRenameWallet() {
    if (QFile::exists(databaseFileName)) {
        QFile::remove(databaseFileName);
    }
    WalletNamesDbStorage db;
    db.init();

    db.giveNameWallet("123", "name1");
    WalletInfo wallet;
    wallet.address = "123";
    wallet.name = "name1";
    wallet.infos.emplace_back("u1", "d1", "c1");
    wallet.infos.emplace_back("u2", "d2", "c2");
    wallet.infos.emplace_back("u3", "d3", "c3");
    db.updateWalletInfo(wallet.address, wallet.infos);

    db.giveNameWallet("123", "name2");
    wallet.name = "name2";

    std::vector<WalletInfo> wallets = db.getAllWallets();

    QCOMPARE(wallets.size(), 1);

    checkInfo(wallets[0], wallet);
}

QTEST_MAIN(tst_WalletNamesDBStorage)
