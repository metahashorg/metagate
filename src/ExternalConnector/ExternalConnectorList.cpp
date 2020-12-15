#include "ExternalConnectorList.h"

#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QDebug>

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("EXTCONN");

namespace {
    const QString ListFileName = QLatin1String(":/ExternalConnectorList.txt");
}

ExternalConnectorList::ExternalConnectorList()
{
    load();
}

bool ExternalConnectorList::checkUrl(const QString &url) const
{
    const QUrl purl(url);
    qDebug() << purl.host() << domains;
    return std::any_of(domains.cbegin(), domains.cend(), [host = purl.host()](const QString &d){
        return d == host;
    });
}

void ExternalConnectorList::load()
{
    if (!QFile::exists(ListFileName)) {
        LOG << "List file not found " << ListFileName;
        return;
    }
    domains.clear();
    QFile file(ListFileName);
    CHECK(file.open(QIODevice::ReadOnly | QIODevice::Text), "can't open file")
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty())
            domains.append(line);
    }
    file.close();
}
