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

};

#endif // TST_RSA_H
