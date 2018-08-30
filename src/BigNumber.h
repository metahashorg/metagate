#ifndef BIGNUMBER_H
#define BIGNUMBER_H

#include <QByteArray>
#include <openssl/bn.h>
#include <memory>
#include <functional>

class BigNumber
{
public:
    BigNumber();
    BigNumber(const QByteArray &dec);
    BigNumber(const BigNumber &bn);

    void setDecimal(const QByteArray &dec);
    QByteArray getDecimal() const;

    BigNumber &operator=(const BigNumber &rhs);
    BigNumber &operator+=(const BigNumber &rhs);
    BigNumber &operator-=(const BigNumber &rhs);

private:
    std::unique_ptr<BIGNUM, std::function<void(BIGNUM *)>> ptr;
};

const BigNumber operator+(const BigNumber &lhs, const BigNumber &rhs);
const BigNumber operator-(const BigNumber &lhs, const BigNumber &rhs);

#endif // BIGNUMBER_H
