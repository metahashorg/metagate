#ifndef BIGNUMBER_H
#define BIGNUMBER_H

#include <QByteArray>
#include <openssl/bn.h>
#include <memory>
#include <functional>

class BigNumber
{
    friend const BigNumber operator+(const BigNumber &lhs, const BigNumber &rhs);
public:
    BigNumber();
    BigNumber(const QByteArray &dec);
    BigNumber(const BigNumber &bn);

    QByteArray getDecimal() const;

private:
    std::unique_ptr<BIGNUM, std::function<void(BIGNUM *)>> ptr;
};

const BigNumber operator+(const BigNumber &lhs, const BigNumber &rhs);

#endif // BIGNUMBER_H
