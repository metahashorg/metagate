#include "BigNumber.h"

#include "check.h"

BigNumber::BigNumber()
    : ptr(BN_new(), BN_free)
{

}

BigNumber::BigNumber(const QByteArray &dec)
    : BigNumber()
{
    BIGNUM *p = ptr.get();
    QByteArray str = dec + '\0';
    BN_dec2bn(&p, str.data());
}

BigNumber::BigNumber(const BigNumber &bn)
    : ptr(BN_dup(bn.ptr.get()), BN_free)
{

}

void BigNumber::setDecimal(const QByteArray &dec)
{
    BIGNUM *p = ptr.get();
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
