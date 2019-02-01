#ifndef TST_RSA_H
#define TST_RSA_H

#include <QObject>

class tst_rsa : public QObject
{
    Q_OBJECT
public:
    explicit tst_rsa(QObject *parent = nullptr);

private slots:

    void testSsl_data();
    void testSsl();

    void testSslMessageError_data();
    void testSslMessageError();

    void testSslWalletError_data();
    void testSslWalletError();

    void testSslIncorrectPassword_data();
    void testSslIncorrectPassword();

};

#endif // TST_RSA_H
