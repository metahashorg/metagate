#ifndef _BTC_TX_
#define _BTC_TX_

#include <string>
#include <vector>

struct Input
{
    std::string wif;
    std::string spendtxid;
    uint32_t spendoutnum;
    std::string scriptPubkey;
    uint64_t outBalance;
};

std::string BuildBTCTransaction(const std::vector<Input>& inputs, uint64_t fee,
                                uint64_t transferAmount, std::string receiveAddress, bool isTestnet);

struct TransferInfo
{
    std::string privkey;
    std::string pubkey;
    std::string spendtxid;
    uint32_t outnum;
    std::string scriptpubkey;
    uint64_t outBalance;
    std::string receiveAddress;
    size_t inscriptoffset;//Полный размер т.е. вместе с полем размера
    size_t inscriptsize;
};

class BTCTransaction
{
public:
    BTCTransaction(bool isTn = true)
    {
        isTestnet = isTn;
        char hs[] = {0x01, 0x00, 0x00, 0x00};
        uint8_t seq[] = {0xFF, 0xFF, 0xFF, 0xFF};
        char lt[] = {0x00, 0x00, 0x00, 0x00};
        hashcodetype = std::string(hs, 4);
        obhashcodetype = std::string(hs, 1);
        version = std::string(hs, 4);
        sequence = std::string((char*)seq, 4);
        locktime = std::string(lt, 4);
    }
    void AddTransfer(
                        const std::string& wif,
                        const std::string& spendtxid,
                        uint32_t spendoutnum,
                        std::string scriptPubkey,
                        uint64_t outBalance,
                        std::string receiveAddress
                    );
    std::string BuildTransaction(uint64_t fee, uint64_t transferAmount);

private:
    std::string buildSignedDump(uint64_t fee, uint64_t transferAmount);
    std::string signAllInputs(const std::string& signingdump);
    std::string removeScripts(const std::string& signingdump, size_t scriptIdx, const std::vector<TransferInfo>& ti);

    std::vector<TransferInfo> m_Transfers;
    std::string hashcodetype;
    std::string obhashcodetype;
    std::string version;
    std::string sequence;
    std::string locktime;
    bool isTestnet;
};

std::string calcHashTxNotWitness(const std::string &tx);

#endif
