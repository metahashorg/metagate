#include "BigNumber.h"

#include <QByteArray>

#include <openssl/bn.h>

#include "check.h"

BigNumber::BigNumber()
    : ptr(BN_new(), BN_free)
{

}

BigNumber::BigNumber(const QByteArray &dec)
    : BigNumber()
{
    setDecimal(dec);
}

BigNumber::BigNumber(const QString &dec)
    : BigNumber()
{
    setDecimal(dec.toUtf8());
}

BigNumber::BigNumber(const BigNumber &bn)
    : ptr(BN_dup(bn.ptr.get()), BN_free)
{

}

void BigNumber::setDecimal(const QByteArray &dec)
{
    BIGNUM *p = ptr.get();
    if (dec.isEmpty()) {
        BN_zero(p);
        return;
    }
    QByteArray str = dec + '\0';
    BN_dec2bn(&p, str.data());
}

QByteArray BigNumber::getDecimal() const
{
    char *str = BN_bn2dec(ptr.get());
    QByteArray res(str);
    OPENSSL_free(str);
    return res;
}

BigNumber &BigNumber::operator=(const BigNumber &rhs)
{
    CHECK(BN_copy(this->ptr.get(), rhs.ptr.get()) == this->ptr.get(), "BN error");
    return *this;
}

BigNumber &BigNumber::operator+=(const BigNumber &rhs)
{
    CHECK(BN_add(this->ptr.get(), this->ptr.get(), rhs.ptr.get()), "BN error");
    return *this;
}

BigNumber &BigNumber::operator-=(const BigNumber &rhs)
{
    CHECK(BN_sub(this->ptr.get(), this->ptr.get(), rhs.ptr.get()), "BN error");
    return *this;
}

bool BigNumber::operator==(const BigNumber &second) const {
    return BN_cmp(this->ptr.get(), second.ptr.get()) == 0;
}

const BigNumber operator+(const BigNumber &lhs, const BigNumber &rhs)
{
    BigNumber res(lhs);
    res += rhs;
    return res;
}

const BigNumber operator-(const BigNumber &lhs, const BigNumber &rhs)
{
    BigNumber res(lhs);
    res -= rhs;
    return res;
}
