#ifndef QRCODER_H
#define QRCODER_H

#include <QtCore>

class QRCoder
{
public:
    QRCoder() = delete;

    static QByteArray encode(const QByteArray &data);
    static QByteArray decode(const QByteArray &data);
};

#endif // QRCODER_H
