#ifndef TST_QRCODER_H
#define TST_QRCODER_H

#include <QObject>

class tst_QRCoder : public QObject
{
    Q_OBJECT
public:
    explicit tst_QRCoder(QObject *parent = nullptr);

private slots:

    void testQRCoderEncodeDecode_data();
    void testQRCoderEncodeDecode();

};

#endif // TST_QRCODER_H
