#ifndef UTILS_UTILS_H
#define UTILS_UTILS_H

#include <QObject>

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "Network/SimpleClient.h"

struct TypedException;

namespace utils {

class Utils : public ManagerWrapper {
    Q_OBJECT
public:

    using OpenInBrowserCallback = CallbackWrapper<void()>;

    using OpenFolderDialogCallback = CallbackWrapper<void(const QString &folder)>;

    using SaveFileFromUrlCallback = CallbackWrapper<void()>;

    using PrintUrlCallback = CallbackWrapper<void()>;

    using ChooseFileAndLoadCallback = CallbackWrapper<void(const std::string &resultBase64)>;

    using QrEncodeCallback = CallbackWrapper<void(const QString &resultBase64)>;

    using QrDecodeCallback = CallbackWrapper<void(const QString &resultHex)>;

    using ChooseFileCallback = CallbackWrapper<void(const QString &file)>;

    using QuestionCallback = CallbackWrapper<void(bool result)>;

public:
    explicit Utils(QObject *parent = nullptr);

    ~Utils() override;

    void setWidget(QWidget *widget);

    void mvToThread(QThread *th);

signals:

    void openInBrowser(const QString &url, const OpenInBrowserCallback &callback);

    void openFolderDialog(const QString &beginPath, const QString &caption, const OpenFolderDialogCallback &callback);

    void saveFileFromUrl(const QString &url, const QString &saveFileWindowCaption, const QString &filePath, bool openAfterSave, const SaveFileFromUrlCallback &callback);

    void printUrl(const QString &url, const QString &printWindowCaption, const QString &text, const PrintUrlCallback &callback);

    void chooseFileAndLoad(const QString &openFileWindowCaption, const QString &filePath, const ChooseFileAndLoadCallback &callback);

    void qrEncode(const QString &textHex, const QrEncodeCallback &callback);

    void qrDecode(const QString &imageBase64, const QrDecodeCallback &callback);

    void javascriptLog(const QString &message);

    void saveFileDialog(const QString &caption, const QString &beginPath, const ChooseFileCallback &callback);

    void loadFileDialog(const QString &caption, const QString &beginPath, const QString &mask, const ChooseFileCallback &callback);

    void question(const QString &caption, const QString &text, const QuestionCallback &callback);

    void openFolderInStandartExplored(const QString &folder);

private slots:

    void onOpenInBrowser(const QString &url, const OpenInBrowserCallback &callback);

    void onOpenFolderDialog(const QString &beginPath, const QString &caption, const OpenFolderDialogCallback &callback);

    void onSaveFileFromUrl(const QString &url, const QString &saveFileWindowCaption, const QString &filePath, bool openAfterSave, const SaveFileFromUrlCallback &callback);

    void onPrintUrl(const QString &url, const QString &printWindowCaption, const QString &text, const PrintUrlCallback &callback);

    void onChooseFileAndLoad(const QString &openFileWindowCaption, const QString &filePath, const ChooseFileAndLoadCallback &callback);

    void onQrEncode(const QString &textHex, const QrEncodeCallback &callback);

    void onQrDecode(const QString &imageBase64, const QrDecodeCallback &callback);

    void onJavascriptLog(const QString &message);

    void onSaveFileDialog(const QString &caption, const QString &beginPath, const ChooseFileCallback &callback);

    void onLoadFileDialog(const QString &caption, const QString &beginPath, const QString &mask, const ChooseFileCallback &callback);

    void onQuestion(const QString &caption, const QString &text, const QuestionCallback &callback);

    void onOpenFolderInStandartExplored(const QString &folder);

private:

    void openFolderInStandartExploredImpl(const QString &folder);

public slots:

private:

    QWidget *widget_ = nullptr;

    SimpleClient client;

};

} // namespace utils

#endif // UTILS_UTILS_H
