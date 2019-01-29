#ifndef PAGESMAPPINGS_H
#define PAGESMAPPINGS_H

#include <map>
#include <vector>
#include <memory>
#include <set>

#include <QString>

const extern QString METAHASH_URL;
const extern QString APP_URL;

template<typename T>
class Optional {
public:

    Optional() = default;

    Optional(const T &t)
        : isSet(true)
        , t(t)
    {}

    Optional& operator=(const T &t_) {
        isSet = true;
        t = t_;
        return *this;
    }

    bool has_value() const {
        return isSet;
    }

    const T& value() const {
        return t;
    }

private:

    bool isSet = false;
    T t;
};

struct PageInfo {
    bool isApp = false;
    QString page;
    QString printedName;
    bool isExternal;
    bool isDefault = false;
    bool isLocalFile = true;
    QString defaultIp;

    std::vector<QString> ips;
    QString getIp(const std::set<QString> &excludes) const;

    void changeDefaultIp(const QString &ip);

    PageInfo() = default;

    PageInfo(const QString &page, bool isExternal, bool isDefault, bool isLocalFile)
        : page(page)
        , isExternal(isExternal)
        , isDefault(isDefault)
        , isLocalFile(isLocalFile)
    {}

    bool operator==(const PageInfo &second) const {
        return std::make_tuple(isApp, page, printedName, isExternal, isDefault, isLocalFile, ips) == std::make_tuple(second.isApp, second.page, second.printedName, second.isExternal, second.isDefault, second.isLocalFile, second.ips);
    }

    bool operator!=(const PageInfo &second) const {
        return !this->operator==(second);
    }
};

class PagesMappings {
private:

    class Name {
    public:

        explicit Name(const QString &text);

        bool operator<(const Name &second) const;

        bool operator==(const Name &second) const;

        const QString& toString() const;

    private:

        QString name;

    };

public:

    PagesMappings();

    void addMappingsMh(QString mapping);

    void setMappings(QString mapping);

    void setFullPagesPath(QString pagesPath) {
        fullPagesPath = pagesPath;
    }

    bool compareTwoPaths(const QString &path1, const QString &path2) const;

    const PageInfo& getSearchPage() const;

    PageInfo find(const QString &text) const;

    const std::vector<QString>& getDefaultIps() const;

    Optional<QString> findName(const QString &url) const;

    QString getIp(const QString &text, const std::set<QString> &excludes={});

    static QString getHost(const QString &url);

private:

    Optional<PageInfo> findInternal(const QString &url) const;

    void setDefaultIpPage(const QString &name, const QString &ip);

    static int findSlashInternal(const QString &url);

private:

    struct UrlName {
        QString name;

        UrlName() = default;

        /*explicit*/ UrlName(const QString &name);

        bool operator<(const UrlName &second) const;
    };

private:

    std::map<Name, std::shared_ptr<PageInfo>> mappingsPages;

    std::vector<QString> defaultMhIps;
    QString defaultMhIp;

    std::map<UrlName, QString> urlToName;

    PageInfo searchPage;

    QString fullPagesPath;

};

#endif // PAGESMAPPINGS_H
