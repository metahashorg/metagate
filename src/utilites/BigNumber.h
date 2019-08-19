#ifndef BIGNUMBER_H
#define BIGNUMBER_H

#include <QString>
#include <memory>
#include <functional>

class QByteArray;

typedef struct bignum_st BIGNUM;

class BigNumber {
public:
    BigNumber();
    BigNumber(const QByteArray &dec);
    BigNumber(const QString &dec);
    BigNumber(const BigNumber &bn);

    bool isZero() const;
    bool isNegative() const;

    void setDecimal(const QByteArray &dec);
    QByteArray getDecimal() const;

    QString getFracDecimal(int mod) const;

    quint32 divAndModToWord(quint32 w, BigNumber &div) const;

    BigNumber &operator=(const BigNumber &rhs);
    BigNumber &operator+=(const BigNumber &rhs);
    BigNumber &operator-=(const BigNumber &rhs);

    bool operator==(const BigNumber &second) const;

private:
    std::unique_ptr<BIGNUM, std::function<void(BIGNUM *)>> ptr;
};

const BigNumber operator+(const BigNumber &lhs, const BigNumber &rhs);
const BigNumber operator-(const BigNumber &lhs, const BigNumber &rhs);

#endif // BIGNUMBER_H
