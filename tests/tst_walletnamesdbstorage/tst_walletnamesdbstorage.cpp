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
        QFile::remove(databaseFileName);    }
    WalletNamesDbStorage db;
    db.init();

    QCOMPARE(db.giveNameWallet("123", "name1"), false);
    QCOMPARE(db.giveNameWallet("234", "name2"), false);
    QCOMPARE(db.giveNameWallet("345", "name1"), false);
    QCOMPARE(db.giveNameWallet("234", "name3"), true);
    QCOMPARE(db.giveNameWallet("345", "name1"), false);

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
        QCOMPARE(i1.type, i2.type);
    }
}

void tst_WalletNamesDBStorage::testUpdateInfo() {
    if (QFile::exists(databaseFileName)) {
        QFile::remove(databaseFileName);
    }
    WalletNamesDbStorage db;
    db.init();

    WalletInfo walletTwo = db.getWalletInfo("123");
    QCOMPARE(walletTwo.address, "");

    db.giveNameWallet("123", "name1");
    WalletInfo wallet;
    wallet.address = "123";
    wallet.name = "name1";
    wallet.infos.emplace_back("u1", "d1", "c1", WalletInfo::Info::Type::Key);
    wallet.infos.emplace_back("u2", "d2", "c2", WalletInfo::Info::Type::Watch);
    wallet.infos.emplace_back("u3", "d3", "c3", WalletInfo::Info::Type::Key);
    db.updateWalletInfo(wallet.address, wallet.infos);

    WalletInfo wallet01;
    wallet01.address = "123";
    wallet01.name = "name1";
    wallet01.infos.emplace_back("u1", "d1", "c1", WalletInfo::Info::Type::Key);
    db.updateWalletInfo(wallet01.address, wallet01.infos);

    db.giveNameWallet("345", "name2");
    WalletInfo wallet2;
    wallet2.address = "345";
    wallet2.name = "name2";
    wallet2.infos.emplace_back("u4", "d4", "c4", WalletInfo::Info::Type::Key);
    wallet2.infos.emplace_back("u5", "d5", "c5", WalletInfo::Info::Type::Key);
    db.updateWalletInfo(wallet2.address, wallet2.infos);

    WalletInfo wallet3;
    wallet3.address = "678";
    wallet3.name = "";
    wallet3.infos.emplace_back("u7", "d7", "c7", WalletInfo::Info::Type::Watch);
    wallet3.infos.emplace_back("u8", "d8", "c8", WalletInfo::Info::Type::Key);
    db.updateWalletInfo(wallet3.address, wallet3.infos);

    WalletInfo wallet4;
    wallet4.address = "012";
    wallet4.name = "name0";
    db.giveNameWallet("012", "name0");

    std::vector<WalletInfo> wallets = db.getAllWallets();
    std::sort(wallets.begin(), wallets.end(), [](const WalletInfo &first, const WalletInfo &second) {
        return first.address < second.address;
    });

    QCOMPARE(wallets.size(), 3);

    checkInfo(wallets[0], wallet4);
    checkInfo(wallets[1], wallet);
    checkInfo(wallets[2], wallet2);

    WalletInfo walletOne = db.getWalletInfo("123");
    checkInfo(walletOne, wallet);
    WalletInfo walletThree = db.getWalletInfo("012");
    checkInfo(walletThree, wallet4);
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
    wallet.infos.emplace_back("u1", "d1", "c1", WalletInfo::Info::Type::Key);
    wallet.infos.emplace_back("u2", "d2", "c2", WalletInfo::Info::Type::Key);
    wallet.infos.emplace_back("u3", "d3", "c3", WalletInfo::Info::Type::Key);
    db.updateWalletInfo(wallet.address, wallet.infos);

    QCOMPARE(db.getNameWallet("123"), "name1");

    db.giveNameWallet("123", "name2");
    wallet.name = "name2";

    QCOMPARE(db.getNameWallet("123"), "name2");

    std::vector<WalletInfo> wallets = db.getAllWallets();

    QCOMPARE(wallets.size(), 1);

    checkInfo(wallets[0], wallet);

    QCOMPARE(db.getNameWallet("223"), "");
}

void tst_WalletNamesDBStorage::testSelectForCurrency() {
    if (QFile::exists(databaseFileName)) {
        QFile::remove(databaseFileName);
    }
    WalletNamesDbStorage db;
    db.init();

    WalletInfo wallet;
    wallet.address = "123";
    wallet.name = "name1";
    wallet.infos.emplace_back("u1", "d1", "c1", WalletInfo::Info::Type::Key);
    wallet.infos.emplace_back("u2", "d2", "c2", WalletInfo::Info::Type::Watch);
    wallet.infos.emplace_back("u3", "d3", "c3", WalletInfo::Info::Type::Key);
    db.addOrUpdateWallet(wallet);

    WalletInfo wallet2;
    wallet2.address = "234";
    wallet2.name = "name2";
    wallet2.infos.emplace_back("u4", "d4", "c4", WalletInfo::Info::Type::Watch);
    wallet2.infos.emplace_back("u2", "d2", "c2", WalletInfo::Info::Type::Watch);
    wallet2.infos.emplace_back("u5", "d5", "c5", WalletInfo::Info::Type::Key);
    db.addOrUpdateWallet(wallet2);

    WalletInfo wallet3;
    wallet3.address = "345";
    wallet3.name = "name3";
    wallet3.infos.emplace_back("u2", "d1", "c1", WalletInfo::Info::Type::Key);
    db.addOrUpdateWallet(wallet3);

    WalletInfo wallet4;
    wallet4.address = "456";
    wallet4.name = "name4";
    db.addOrUpdateWallet(wallet4);

    std::vector<WalletInfo> wallets = db.getWalletsCurrency("c2", "u2");
    std::sort(wallets.begin(), wallets.end(), [](const WalletInfo &first, const WalletInfo &second) {
        return first.address < second.address;
    });

    QCOMPARE(wallets.size(), 2);
    checkInfo(wallets[0], wallet);
    checkInfo(wallets[1], wallet2);
}

QTEST_MAIN(tst_WalletNamesDBStorage)
