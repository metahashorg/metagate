#ifndef EXTERNALCONNECTORLIST_H
#define EXTERNALCONNECTORLIST_H

#include <QStringList>

class ExternalConnectorList
{
public:
    ExternalConnectorList();

    bool checkUrl(const QString &url) const;

private:
    void load();

    QStringList domains;
};

#endif // EXTERNALCONNECTORLIST_H
