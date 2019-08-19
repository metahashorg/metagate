#include "UtilsManager.h"

#include "qt_utilites/ManagerWrapperImpl.h"

#include "qt_utilites/QRegister.h"
#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "utilites/utils.h"
#include "utilites/qrcoder.h"

#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QMessageBox>

SET_LOG_NAMESPACE("UTIL");

namespace utils {

Utils::Utils() {
    Q_CONNECT(this, &Utils::openInBrowser, this, &Utils::onOpenInBrowser);
    Q_CONNECT(this, &Utils::openFolderDialog, this, &Utils::onOpenFolderDialog);
    Q_CONNECT(this, &Utils::saveFileFromUrl, this, &Utils::onSaveFileFromUrl);
    Q_CONNECT(this, &Utils::printUrl, this, &Utils::onPrintUrl);
    Q_CONNECT(this, &Utils::chooseFileAndLoad, this, &Utils::onChooseFileAndLoad);
    Q_CONNECT(this, &Utils::qrEncode, this, &Utils::onQrEncode);
    Q_CONNECT(this, &Utils::qrDecode, this, &Utils::onQrDecode);
    Q_CONNECT(this, &Utils::javascriptLog, this, &Utils::onJavascriptLog);
    Q_CONNECT(this, &Utils::saveFileDialog, this, &Utils::onSaveFileDialog);
    Q_CONNECT(this, &Utils::loadFileDialog, this, &Utils::onLoadFileDialog);
    Q_CONNECT(this, &Utils::question, this, &Utils::onQuestion);
    Q_CONNECT(this, &Utils::openFolderInStandartExplored, this, &Utils::onOpenFolderInStandartExplored);

    Q_REG(OpenInBrowserCallback, "OpenInBrowserCallback");
    Q_REG(OpenFolderDialogCallback, "OpenFolderDialogCallback");
    Q_REG(SaveFileFromUrlCallback, "SaveFileFromUrlCallback");
    Q_REG(PrintUrlCallback, "PrintUrlCallback");
    Q_REG(ChooseFileAndLoadCallback, "ChooseFileAndLoadCallback");
    Q_REG(QrEncodeCallback, "QrEncodeCallback");
    Q_REG(QrDecodeCallback, "QrDecodeCallback");
    Q_REG(ChooseFileCallback, "ChooseFileCallback");
    Q_REG(QuestionCallback, "QuestionCallback");

    client.setParent(this);
    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &Utils::callbackCall);
}

Utils::~Utils() = default;

void Utils::setWidget(QWidget *widget) {
    widget_ = widget;
}

void Utils::mvToThread(QThread *th) {
    this->moveToThread(th);
    client.moveToThread(th);
}

void Utils::onOpenInBrowser(const QString &url, const OpenInBrowserCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        QDesktopServices::openUrl(QUrl(url));
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onOpenFolderDialog(const QString &beginPath, const QString &caption, const OpenFolderDialogCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return QFileDialog::getExistingDirectory(widget_, caption, beginPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onSaveFileFromUrl(const QString &url, const QString &saveFileWindowCaption, const QString &filePath, bool openAfterSave, const SaveFileFromUrlCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        const QString beginPath = filePath;
        const QString file = QFileDialog::getSaveFileName(widget_, saveFileWindowCaption, beginPath);
        CHECK(!file.isNull() && !file.isEmpty(), "File not changed");

        client.sendMessageGet(url, [this, file, openAfterSave](const SimpleClient::Response &response) {
            CHECK(!response.exception.isSet(), "Error load image: " + response.exception.description);
            writeToFileBinary(file, response.response, false);
            if (openAfterSave) {
                openFolderInStandartExploredImpl(QFileInfo(file).dir().path());
            }
        });
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onPrintUrl(const QString &url, const QString &printWindowCaption, const QString &text, const PrintUrlCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        client.sendMessageGet(url, [printWindowCaption, text](const SimpleClient::Response &response) {
            CHECK(!response.exception.isSet(), "Error load image: " + response.exception.description);

            QImage image;
            image.loadFromData((const unsigned char*)response.response.data(), (int)response.response.size());

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
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onChooseFileAndLoad(const QString &openFileWindowCaption, const QString &filePath, const QString &filters, const ChooseFileAndLoadCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        const QString beginPath = filePath;
        const QString file = QFileDialog::getOpenFileName(widget_, openFileWindowCaption, beginPath, filters);
        const std::string fileData = readFileBinary(file);
        return std::make_tuple(file, toBase64(fileData));
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onQrEncode(const QString &textHex, const QrEncodeCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK_TYPED(!textHex.isEmpty(), TypeErrors::INCORRECT_USER_DATA, "text for encode empty");
        const QByteArray data = QByteArray::fromHex(textHex.toUtf8());
        const QByteArray res = QRCoder::encode(data);
        CHECK_TYPED(res.size() > 0, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect result");
        const QByteArray check = QRCoder::decode(res);
        CHECK_TYPED(check == data, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect check result");
        return QString(res.toBase64());
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onQrDecode(const QString &imageBase64, const QrDecodeCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK_TYPED(!imageBase64.isEmpty(), TypeErrors::INCORRECT_USER_DATA, "text for encode empty");
        const QByteArray data = QByteArray::fromBase64(imageBase64.toUtf8());
        const QByteArray res = QRCoder::decode(data);
        CHECK_TYPED(res.size() > 0, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect result");
        return QString(res.toHex());
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onJavascriptLog(const QString &message) {
BEGIN_SLOT_WRAPPER
    LOG3("ECMA") << message;
END_SLOT_WRAPPER
}

void Utils::onSaveFileDialog(const QString &caption, const QString &beginPath, const ChooseFileCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return QFileDialog::getSaveFileName(widget_, caption, beginPath);
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onLoadFileDialog(const QString &caption, const QString &beginPath, const QString &mask, const ChooseFileCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return QFileDialog::getOpenFileName(widget_, caption, beginPath, mask);
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onQuestion(const QString &caption, const QString &text, const QuestionCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        const QMessageBox::StandardButton reply = QMessageBox::question(widget_, caption, text, QMessageBox::Yes|QMessageBox::No);
        return reply == QMessageBox::Yes;
    }, callback);
END_SLOT_WRAPPER
}

void Utils::onOpenFolderInStandartExplored(const QString &folder) {
BEGIN_SLOT_WRAPPER
    openFolderInStandartExploredImpl(folder);
END_SLOT_WRAPPER
}

void Utils::openFolderInStandartExploredImpl(const QString &folder) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

} // namespace utils
