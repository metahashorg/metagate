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

    Q_INVOKABLE void qtOpenInBrowser(const QString &url);

    Q_INVOKABLE void openFolderDialog(const QString &beginPath, const QString &caption);

    Q_INVOKABLE void saveFileFromUrl(const QString &url, const QString &saveFileWindowCaption, bool openAfterSave, const QString &filePath);

    Q_INVOKABLE void printUrl(const QString &url, const QString &printWindowCaption, const QString &text);

    Q_INVOKABLE void chooseFileAndLoad2(const QString &openFileWindowCaption, const QString &filePath);

    Q_INVOKABLE void qrEncode(const QString &textHex);

    Q_INVOKABLE void qrDecode(const QString &imageBase64);

    Q_INVOKABLE void javascriptLog(const QString &message);

private:

    Utils &manager;
};

} // namespace utils

#endif // UTILSJAVASCRIPT_H
