#include "UtilsManager.h"

#include "QRegister.h"
#include "check.h"
#include "SlotWrapper.h"
#include "utils.h"
#include "qrcoder.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>

SET_LOG_NAMESPACE("UTIL");

namespace utils {

Utils::Utils(QObject *parent) : QObject(parent) {
    Q_CONNECT(this, &Utils::callbackCall, this, &Utils::onCallbackCall);
    Q_CONNECT(this, &Utils::openInBrowser, this, &Utils::onOpenInBrowser);
    Q_CONNECT(this, &Utils::openFolderDialog, this, &Utils::onOpenFolderDialog);
    Q_CONNECT(this, &Utils::saveFileFromUrl, this, &Utils::onSaveFileFromUrl);
    Q_CONNECT(this, &Utils::printUrl, this, &Utils::onPrintUrl);
    Q_CONNECT(this, &Utils::chooseFileAndLoad, this, &Utils::onChooseFileAndLoad);
    Q_CONNECT(this, &Utils::qrEncode, this, &Utils::onQrEncode);
    Q_CONNECT(this, &Utils::qrDecode, this, &Utils::onQrDecode);
    Q_CONNECT(this, &Utils::javascriptLog, this, &Utils::onJavascriptLog);

    Q_REG(Utils::Callback, "Utils::Callback");
    Q_REG(OpenInBrowserCallback, "OpenInBrowserCallback");
    Q_REG(OpenFolderDialogCallback, "OpenFolderDialogCallback");
    Q_REG(SaveFileFromUrlCallback, "SaveFileFromUrlCallback");
    Q_REG(PrintUrlCallback, "PrintUrlCallback");
    Q_REG(ChooseFileAndLoadCallback, "ChooseFileAndLoadCallback");
    Q_REG(QrEncodeCallback, "QrEncodeCallback");
    Q_REG(QrDecodeCallback, "QrDecodeCallback");

    client.setParent(this);
    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &Utils::callbackCall);
}

void Utils::setWidget(QWidget *widget) {
    widget_ = widget;
}

void Utils::mvToThread(QThread *th) {
    this->moveToThread(th);
    client.moveToThread(th);
}

void Utils::onCallbackCall(Callback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void Utils::onOpenInBrowser(const QString &url, const OpenInBrowserCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        QDesktopServices::openUrl(QUrl(url));
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void Utils::onOpenFolderDialog(const QString &beginPath, const QString &caption, const OpenFolderDialogCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString dir;
    const TypedException &exception = apiVrapper2([&]{
        dir = QFileDialog::getExistingDirectory(widget_, caption, beginPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    });
    callback.emitFunc(exception, dir);
END_SLOT_WRAPPER
}

void Utils::onSaveFileFromUrl(const QString &url, const QString &saveFileWindowCaption, const QString &filePath, bool openAfterSave, const SaveFileFromUrlCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        const QString beginPath = filePath;
        const QString file = QFileDialog::getSaveFileName(widget_, saveFileWindowCaption, beginPath);
        CHECK(!file.isNull() && !file.isEmpty(), "File not changed");

        client.sendMessageGet(url, [this, file, openAfterSave](const std::string &response, const SimpleClient::ServerException &exception) {
            CHECK(!exception.isSet(), "Error load image: " + exception.description);
            writeToFileBinary(file, response, false);
            if (openAfterSave) {
                openFolderInStandartExplored(QFileInfo(file).dir().path());
            }
        });
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void Utils::onPrintUrl(const QString &url, const QString &printWindowCaption, const QString &text, const PrintUrlCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        client.sendMessageGet(url, [printWindowCaption, text](const std::string &response, const SimpleClient::ServerException &exception) {
            CHECK(!exception.isSet(), "Error load image: " + exception.description);

            QImage image;
            image.loadFromData((const unsigned char*)response.data(), (int)response.size());

            QPrinter printer;

            QPrintDialog *dialog = new QPrintDialog(&printer);
            dialog->setWindowTitle(printWindowCaption);

            if (dialog->exec() != QDialog::Accepted) {
                return;
            }

            QPainter painter;
            painter.begin(&printer);

            const int printerWidth = printer.pageRect().width();
            const int printerHeight = printer.pageRect().height();
            const int imageWidth = image.size().width();
            const int imageHeight = image.size().height();
            const int paddingX = (printerWidth - imageWidth) / 2;
            const int paddingY = (printerHeight - imageHeight) / 2;

            painter.drawText(100, 100, 500, 500, Qt::AlignLeft|Qt::AlignTop, text);
            painter.drawImage(QRect(paddingX, paddingY, imageWidth, imageHeight), image);

            painter.end();
        });
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void Utils::onChooseFileAndLoad(const QString &openFileWindowCaption, const QString &filePath, const ChooseFileAndLoadCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::string base64Data;
    const TypedException &exception = apiVrapper2([&]{
        const QString beginPath = filePath;
        const QString file = QFileDialog::getOpenFileName(widget_, openFileWindowCaption, beginPath);
        const std::string fileData = readFileBinary(file);
        base64Data = toBase64(fileData);
    });
    callback.emitFunc(exception, base64Data);
END_SLOT_WRAPPER
}

void Utils::onQrEncode(const QString &textHex, const QrEncodeCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString base64Data;
    const TypedException &exception = apiVrapper2([&]{
        CHECK_TYPED(!textHex.isEmpty(), TypeErrors::INCORRECT_USER_DATA, "text for encode empty");
        const QByteArray data = QByteArray::fromHex(textHex.toUtf8());
        const QByteArray res = QRCoder::encode(data);
        CHECK_TYPED(res.size() > 0, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect result");
        const QByteArray check = QRCoder::decode(res);
        CHECK_TYPED(check == data, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect check result");
        base64Data = QString(res.toBase64());
    });
    callback.emitFunc(exception, base64Data);
END_SLOT_WRAPPER
}

void Utils::onQrDecode(const QString &imageBase64, const QrDecodeCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString result;
    const TypedException &exception = apiVrapper2([&]{
        CHECK_TYPED(!imageBase64.isEmpty(), TypeErrors::INCORRECT_USER_DATA, "text for encode empty");
        const QByteArray data = QByteArray::fromBase64(imageBase64.toUtf8());
        const QByteArray res = QRCoder::decode(data);
        CHECK_TYPED(res.size() > 0, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect result");
        result = QString(res.toHex());
    });
    callback.emitFunc(exception, result);
END_SLOT_WRAPPER
}

void Utils::onJavascriptLog(const QString &message) {
BEGIN_SLOT_WRAPPER
    LOG3("ECMA") << message;
END_SLOT_WRAPPER
}

void Utils::openFolderInStandartExplored(const QString &folder) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

} // namespace utils