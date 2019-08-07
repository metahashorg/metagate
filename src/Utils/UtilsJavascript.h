#ifndef UTILSJAVASCRIPT_H
#define UTILSJAVASCRIPT_H

#include <functional>

#include "qt_utilites/WrapperJavascript.h"

namespace utils {

class Utils;

class UtilsJavascript: public WrapperJavascript {
    Q_OBJECT
public:

    UtilsJavascript(Utils &manager);

public slots:

    Q_INVOKABLE void qtOpenInBrowser(const QString &url, const QString &callback);

    Q_INVOKABLE void openFolderDialog(const QString &beginPath, const QString &caption, const QString &callback);

    Q_INVOKABLE void openFileDialog(const QString &beginPath, const QString &caption, const QString &filters, const QString &callback);

    Q_INVOKABLE void saveFileFromUrl2(const QString &url, const QString &saveFileWindowCaption, bool openAfterSave, const QString &filePath, const QString &callback);

    Q_INVOKABLE void printUrl(const QString &url, const QString &printWindowCaption, const QString &text, const QString &callback);

    Q_INVOKABLE void chooseFileAndLoad(const QString &openFileWindowCaption, const QString &filePath, const QString &callback);

    Q_INVOKABLE void qrEncode(const QString &textHex, const QString &callback);

    Q_INVOKABLE void qrDecode(const QString &imageBase64, const QString &callback);

    Q_INVOKABLE void javascriptLog(const QString &message);

private:

    Utils &manager;
};

} // namespace utils

#endif // UTILSJAVASCRIPT_H
