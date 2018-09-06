#include "qrcoder.h"

#include "zbar.h"
#include "QrCode.hpp"

#include <QImage>
#include <QPainter>
#include <QBuffer>

#include "check.h"

QByteArray QRCoder::encode(const QByteArray &data)
{
    QByteArray res;
    const int s = 10;
    const QColor color(Qt::black);
    const qrcodegen::QrCode::Ecc ecl = qrcodegen::QrCode::Ecc::MEDIUM;
    const std::vector<uint8_t> vec(data.begin(), data.end());
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeBinary(vec, ecl);
    const int is = s * (qr.getSize() + 6);
    QImage image(is, is, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter p(&image);
    p.setPen(Qt::NoPen);
    for(int y = 0; y < qr.getSize(); y++) {
        for(int x = 0; x < qr.getSize(); x++) {
            if(qr.getModule(x, y) != 0) {
                p.fillRect(s  * (x + 3), s * (y + 3), s, s, color);
            }
        }
    }

    QBuffer buffer(&res);
    CHECK(buffer.open(QIODevice::WriteOnly), "Open error");
    CHECK(image.save(&buffer, "PNG"), "Save error");
    return res;
}

QByteArray QRCoder::decode(const QByteArray &data)
{
    QImage image;
    CHECK(image.loadFromData(data, "PNG"), "Load image error");
    image = image.convertToFormat(QImage::Format_Grayscale8);

    zbar::Image zimage(static_cast<unsigned int>(image.bytesPerLine()), static_cast<unsigned int>(image.height()), "Y800", image.bits(), static_cast<unsigned long>(image.sizeInBytes()));
    zbar::ImageScanner scanner;

    // Configure scanner
    scanner.set_config(zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ENABLE, 1);
    scanner.scan(zimage);

    for(zbar::Image::SymbolIterator symbol = zimage.symbol_begin(); symbol != zimage.symbol_end(); ++symbol)
    {
        if (symbol->get_type() != zbar::ZBAR_QRCODE)
            continue;
        return QByteArray::fromStdString(symbol->get_data());
    }
    return QByteArray();
}
