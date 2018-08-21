#include "bignumber.h"

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
      /*BN_dec2bn(&p, p_str);

      char * number_str = BN_bn2hex(p);
      printf("%s\n", number_str);

      OPENSSL_free(number_str);
      BN_free(p); int BN_add(BIGNUM *r, const BIGNUM *a, const BIGNUM *b);

*/
}

BigNumber::BigNumber(const BigNumber &bn)
    : ptr(BN_dup(bn.ptr.get()), BN_free)
{

}

QByteArray BigNumber::getDecimal() const
{
    char *str = BN_bn2dec(ptr.get());
    QByteArray res(str);
    OPENSSL_free(str);
    return res;
}

const BigNumber operator+(const BigNumber &lhs, const BigNumber &rhs)
{
    BigNumber res;
    int r = BN_add(res.ptr.get(), lhs.ptr.get(), rhs.ptr.get());
    return res;
}
