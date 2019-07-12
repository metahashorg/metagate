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

#include <QDebug>
QString BigNumber::getFracDecimal(int mod) const
{
    quint32 w = 1;
    for (int i = 0; i < mod; i++)
        w *= 10;
    BigNumber p;
    quint32 q = divAndModToWord(w, p);
    QString ret = QString::fromLatin1(p.getDecimal());
    QString sq = QString::number(q);
    sq = sq.rightJustified(mod, '0');
    int n = mod - 1;
    for (; n >= 0; n--)
        if (sq.at(n) != '0')
            break;
    sq = sq.left(n + 1);
    if (!sq.isEmpty())
        ret += QStringLiteral(".") + sq;
    return ret;
}

quint32 BigNumber::divAndModToWord(quint32 w, BigNumber &div) const
{
    div = *this;
    quint32 mod = BN_div_word(div.ptr.get(), w);
    CHECK(mod != (BN_ULONG)(-1), "BN error");
    return mod;
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
