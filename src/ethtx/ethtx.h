#ifndef ETH_TX_H_ 
#define ETH_TX_H_

#include <string>

#include <secp256k1/include/secp256k1.h>

secp256k1_context const* getCtx();

std::string SignTransaction(std::string rawprivkey,
                            std::string nonce,
                            std::string gasPrice,
                            std::string gasLimit,
                            std::string to,
                            std::string value,
                            std::string data);

std::string createHashTx(const std::string &tx);

#endif // ETH_TX_H_
