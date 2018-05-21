#include "btctx.h"

#include <algorithm>
#include "cryptopp/sha.h"
#include "secp256k1/include/secp256k1_recovery.h"

#include <iostream>

#include "check.h"

#include "wif.h"
#include "ethtx/utils2.h"
#include "ethtx/const.h"

secp256k1_context const* getCtx();

void BTCTransaction::AddTransfer(
    const std::string& wif,
    const std::string& spendtxid,
    uint32_t spendoutnum,
    std::string scriptPubkey,
    uint64_t outBalance,
    std::string receiveAddress
) {
    CHECK(!wif.empty(), "wif empty");

    bool isCompressed = false;
    TransferInfo transfer;
    transfer.privkey = WIFToPrivkey(wif, isCompressed);
    CHECK(!transfer.privkey.empty(), "privkey empty");
    if (isCompressed) {
        transfer.pubkey = PrivKeyToCompressedPubKey(transfer.privkey);
    } else {
        transfer.pubkey = PrivKeyToPubKey(transfer.privkey);
    }
    CHECK(!transfer.pubkey.empty(), "pubkey empty");
    transfer.spendtxid = spendtxid;
    std::reverse(transfer.spendtxid.begin(), transfer.spendtxid.end());
    transfer.outnum = spendoutnum;
    transfer.scriptpubkey = scriptPubkey;
    transfer.outBalance = outBalance;
    transfer.receiveAddress = receiveAddress;

    m_Transfers.push_back(transfer);
}

std::string BTCTransaction::BuildTransaction(uint64_t fee, uint64_t transferAmount) {
    //Собираем дамп для подписи
    const std::string signingdump = buildSignedDump(fee, transferAmount);
    return signAllInputs(signingdump);
}

std::string BTCTransaction::doubleHash(const std::string& str) {
    //2 раза подсчитываем sha256-хэш от строки
    CryptoPP::SHA256 sha256;
    uint8_t sha256hash[CryptoPP::SHA256::DIGESTSIZE] = {0};
    sha256.CalculateDigest(sha256hash, (uint8_t*)str.data(), str.size());
    //Подсчитываем хэш от хеша
    uint8_t sha256hashfinal[CryptoPP::SHA256::DIGESTSIZE] = {0};
    sha256.CalculateDigest(sha256hashfinal, sha256hash, CryptoPP::SHA256::DIGESTSIZE);
    return std::string((char*)sha256hashfinal, CryptoPP::SHA256::DIGESTSIZE);
}

std::string BTCTransaction::removeScripts(const std::string& signingdump, size_t scriptIdx, const std::vector<TransferInfo>& ti) {
    std::string dump = "";
    size_t pos = 0;
    for (size_t i = 0; i < ti.size(); ++i) {
        //В новую строку копируем все до следующего скрипта в inputе
        CHECK(signingdump.size() >= ti[i].inscriptoffset, "Incorrect signingdump");
        dump += signingdump.substr(pos, ti[i].inscriptoffset - pos);
        pos = ti[i].inscriptoffset;
        //Если это удаляемый скрипт, то заменяем его на символ 0x00
        if (i != scriptIdx) {
            dump.push_back(0);
        } else {//если искомый скрипт, то копируем в неизменном виде
            CHECK(signingdump.size() >= ti[i].inscriptsize + pos, "Incorrect signingdump");
            dump += signingdump.substr(pos, ti[i].inscriptsize);
        }
        pos += ti[i].inscriptsize;
    }
    //Добавляем остаток строки
    size_t remoffset = ti[ti.size()-1].inscriptoffset + ti[ti.size()-1].inscriptsize;
    dump += signingdump.substr(remoffset, signingdump.size()-remoffset);
    return dump;
}

std::string BTCTransaction::buildSignedDump(uint64_t fee, uint64_t transferAmount) {
    //Версия
    std::string dump;

    dump += version;
    uint64_t incount, outcount;
    incount = outcount = 0;   
    const uint64_t fullAmount = transferAmount;
    uint64_t fullBalance = 0;
    for (size_t i = 0; i < m_Transfers.size(); ++i) {
        ++incount;
        fullBalance += m_Transfers[i].outBalance;
    }
    CHECK(fullBalance >= fullAmount + fee, "Not enough money");
    const uint64_t fullChange = fullBalance - fullAmount - fee;
    outcount = (fullChange > 0) ? 2:1;
    //Колл-во intputов
    dump += PackInteger(incount);
    //Заполняем секцию inputов
    for (size_t i = 0; i < m_Transfers.size(); ++i) {
        //Заполняем outpoint
        dump += m_Transfers[i].spendtxid;
        dump += IntegerToBuffer(m_Transfers[i].outnum);
        //Сохраняем смещение pubscriptkey
        const uint64_t val = m_Transfers[i].scriptpubkey.size();
        const std::string varint = PackInteger(val);
        m_Transfers[i].inscriptoffset = dump.size();
        m_Transfers[i].inscriptsize = val + varint.size();
        //Добавляем ключ
        dump += varint;
        dump += m_Transfers[i].scriptpubkey;
        dump += sequence;
    }
    dump += PackInteger(outcount);

    //out, соответствующий переводу по адресу
    dump += IntegerToBuffer(fullAmount);
    const std::string outpubkeyscript = AddressToPubkeyScript(m_Transfers[0].receiveAddress);//Т.к. по условию адресат один
    const std::string varint = PackInteger(outpubkeyscript.size());
    dump += varint;
    dump += outpubkeyscript;
    //Добавляем out сдачи если она есть
    if (outcount == 2) {
        dump += IntegerToBuffer(fullChange);
        //Вычисляем pubkeyscript отправителя
        std::string senderaddr;
        if (m_Transfers[0].pubkey.size() == EC_PUB_KEY_LENGTH) {//Т.к. по условию отправитель один
            senderaddr = PubkeyToAddress(m_Transfers[0].pubkey, isTestnet);
        } else {
            senderaddr = CompressedPubkeyToAddress(m_Transfers[0].pubkey, isTestnet);
        }
        CHECK(senderaddr.size() >= 21, "Incorrect senderaddr");
        const std::string outpubkeyscript2 = HexStringToDump("76a914") + std::string((char*)senderaddr.c_str()+1, 20) + HexStringToDump("88ac");
        dump += PackInteger(outpubkeyscript2.size());
        dump += outpubkeyscript2;
    }

    //Добавляем хэш-код
    dump += locktime;
    dump += hashcodetype;

    return dump;
}

std::string BTCTransaction::signAllInputs(const std::string& signingdump)
{
    std::string transaction = signingdump;
    std::string copyTransaction = transaction;
    const std::vector<TransferInfo> copyTransfers = m_Transfers;

    //Подписываем каждый input
    for (size_t i = 0; i < m_Transfers.size(); ++i) {
        //Создаем дамп для подписи конкретного inputа
        const std::string signtemplate = removeScripts(copyTransaction, i, copyTransfers);
        //Подсчитываем хэш от дампа
        const std::string sha256hash = doubleHash(signtemplate);
        //Рассчитывает сигнатуру для конкретного ключа
        secp256k1_ecdsa_signature sig;
        const bool res = secp256k1_ecdsa_sign(getCtx(), &sig, (const uint8_t*)sha256hash.c_str(),
                                (const uint8_t*)m_Transfers[i].privkey.c_str(),
                                nullptr, NULL);
        CHECK(res, "not sign");
        size_t signbufsize = 256;
        uint8_t signbuf[256] = {};
        const bool res2 = secp256k1_ecdsa_signature_serialize_der(getCtx(), signbuf, &signbufsize, &sig);
        CHECK(res2, "not sign");
        std::string signature = std::string((char*)signbuf, signbufsize);
        signature += obhashcodetype;
        uint8_t opcode = (uint8_t)signature.size();
        std::string inputscript = std::string((char*)&opcode, 1);
        inputscript += signature;
        opcode = (uint8_t)m_Transfers[i].pubkey.size();
        inputscript += std::string((char*)&opcode, 1);
        inputscript += m_Transfers[i].pubkey;
        opcode = (uint8_t)inputscript.size();
        inputscript = std::string((char*)&opcode, 1) + inputscript;
        //Устанавливаем сигнатуру в input
        transaction.replace(m_Transfers[i].inscriptoffset, m_Transfers[i].inscriptsize, inputscript);
        //Пересчитываем смещения
        const size_t delta = inputscript.size() - m_Transfers[i].inscriptsize;
        for (size_t j = i + 1; j < m_Transfers.size(); ++j) {
            m_Transfers[j].inscriptoffset += delta;
        }
    }
    //Удаляем хэшкод (4 байта с конца)
    transaction = transaction.substr(0, transaction.size()-4);
    return transaction;
}


std::string BuildBTCTransaction(
    const std::vector<Input>& inputs, uint64_t fee,
    uint64_t transferAmount, std::string receiveAddress, bool isTestnet
) {
    BTCTransaction transaction(isTestnet);
    for (size_t i = 0; i < inputs.size(); ++i) {
        transaction.AddTransfer(
            inputs[i].wif,
            inputs[i].spendtxid,
            inputs[i].spendoutnum,
            inputs[i].scriptPubkey,
            inputs[i].outBalance,
            receiveAddress
        );
    }
    return transaction.BuildTransaction(fee, transferAmount);
}
