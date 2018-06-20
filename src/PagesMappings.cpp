#include "PagesMappings.h"

#include "Log.h"
#include "check.h"
#include "utils.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

const QString METAHASH_URL = "mh://";
const QString APP_URL = "app://";

PagesMappings::PagesMappings(){

}

static QString concatenateTwoPath(const QString &path1, const QString &path2) {
    QString path = path1;
    if (path.endsWith('/')) {
        path = path.left(path.size() - 1);
    }
    return path + path2;
}

PagesMappings::Name::Name(const QString &text) {
    QString txt = text;
    if (txt.endsWith('/')) {
        txt = txt.left(txt.size() - 1);
    }
    name = txt.toLower();
}

bool PagesMappings::Name::operator<(const Name &second) const {
    return this->name < second.name;
}

bool PagesMappings::Name::operator==(const Name &second) const {
    return this->name == second.name;
}

static QString ipToHttp(const QString &ip) {
    const static QString HTTP = "http://";
    if (ip.startsWith(HTTP)) {
        return ip;
    } else {
        return HTTP + ip;
    }
}

void PagesMappings::setMappingsMh(QString mapping) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(mapping.toUtf8(), &parseError);
    CHECK(parseError.error == QJsonParseError::NoError, "Json parse error: " + parseError.errorString().toStdString());
    const QJsonObject root = document.object();
    CHECK(root.contains("ext") && root.value("ext").isArray(), "ext field not found ");
    const QJsonArray &routes = root.value("ext").toArray();
    for (const QJsonValue &value: routes) {
        CHECK(value.isObject(), "value array incorrect type");
        const QJsonObject &element = value.toObject();
        CHECK(element.contains("type") && element.value("type").isString(), "type field not found");
        const QString type = element.value("type").toString();
        const bool isDefault = type == "defaultGateway";
        if (!isDefault) {
            CHECK(element.contains("url") && element.value("url").isString(), "url field not found");
            const QString url = element.value("url").toString();
            CHECK(element.contains("name") && element.value("name").isString(), "name field not found");
            const QString name = element.value("name").toString();
            CHECK(element.contains("isExternal") && element.value("isExternal").isBool(), "isExternal field not found");
            const bool isExternal = element.value("isExternal").toBool();

            std::shared_ptr<PageInfo> page = std::make_shared<PageInfo>(url, isExternal, isDefault, false);
            page->printedName = METAHASH_URL + name;

            if (element.contains("ip") && element.value("ip").isArray()) {
                for (const QJsonValue &ip: element.value("ip").toArray()) {
                    CHECK(ip.isString(), "ips array incorrect type");
                    const QString ipHttp = ipToHttp(ip.toString());
                    page->ips.emplace_back(ipHttp);

                    auto foundUrlToName = urlToName.find(ipHttp);
                    if (foundUrlToName == urlToName.end()) {
                        urlToName[ipHttp] = page->printedName;
                    }
                }
            }

            auto addToMap = [this](auto &map, const QString &key, const std::shared_ptr<PageInfo> &page) {
                const Name name(key);
                auto found = map.find(name);
                if (found == map.end() || found->second->page.startsWith(METAHASH_URL)) { // Данные из javascript имеют приоритет
                    map[name] = page;
                }
            };

            addToMap(mappingsPages, name, page);
            addToMap(mappingsPages, url, page);
            addToMap(mappingsPages, page->printedName, page);

            if (element.contains("aliases") && element.value("aliases").isArray()) {
                for (const QJsonValue &alias: element.value("aliases").toArray()) {
                    CHECK(alias.isString(), "aliases array incorrect type");
                    addToMap(mappingsPages, alias.toString(), page);
                }
            }
        } else {
            if (element.contains("ip") && element.value("ip").isArray()) {
                for (const QJsonValue &ip: element.value("ip").toArray()) {
                    CHECK(ip.isString(), "ips array incorrect type");
                    const QString ipHttp = ipToHttp(ip.toString());
                    defaultMhIps.emplace_back(ipHttp);
                }
            }
        }
    }
}

void PagesMappings::setMappings(QString mapping) {
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(mapping.toUtf8(), &parseError);
    CHECK(parseError.error == QJsonParseError::NoError, "Json parse error: " + parseError.errorString().toStdString());
    const QJsonObject root = document.object();
    CHECK(root.contains("routes") && root.value("routes").isArray(), "routes field not found");
    const QJsonArray &routes = root.value("routes").toArray();
    for (const QJsonValue &value: routes) {
        CHECK(value.isObject(), "value array incorrect type");
        const QJsonObject &element = value.toObject();
        CHECK(element.contains("url") && element.value("url").isString(), "url field not found");
        const QString url = element.value("url").toString();
        CHECK(element.contains("name") && element.value("name").isString(), "name field not found");
        const QString name = element.value("name").toString();
        CHECK(element.contains("isExternal") && element.value("isExternal").isBool(), "isExternal field not found");
        const bool isExternal = element.value("isExternal").toBool();
        bool isDefault = false;
        if (element.contains("isDefault") && element.value("isDefault").isBool()) {
            isDefault = element.value("isDefault").toBool();
        }
        bool isPreferred = false;
        if (element.contains("isPreferred") && element.value("isPreferred").isBool()) {
            isPreferred = element.value("isPreferred").toBool();
        }
        bool isLocalFile = true;
        if (element.contains("isLocalFile") && element.value("isLocalFile").isBool()) {
            isLocalFile = element.value("isLocalFile").toBool();
        }

        std::shared_ptr<PageInfo> pageInfo = std::make_shared<PageInfo>(url, isExternal, isDefault, isLocalFile);
        if (!name.startsWith(APP_URL) && !name.startsWith(METAHASH_URL)) {
            pageInfo->printedName = APP_URL + name;
        } else {
            pageInfo->printedName = name;
        }
        mappingsPages[Name(name)] = pageInfo;

        if (isDefault) {
            searchPage = *pageInfo;
        }

        auto foundUrlToName = urlToName.find(url);
        if (foundUrlToName == urlToName.end() || isPreferred) {
            urlToName[url] = pageInfo->printedName;
        }
    }
}

struct PathParsed {
    enum class Type {
        METAHASH, APP, NONE
    };

    QString path;

    Type type;

    PathParsed(const QString &url) {
        if (url.startsWith(METAHASH_URL)) {
            type = Type::METAHASH;
            path = url.mid(METAHASH_URL.size());
        } else if (url.startsWith(APP_URL)) {
            type = Type::APP;
            path = url.mid(APP_URL.size());
        } else {
            type = Type::NONE;
            path = url;
        }
    }
};

bool PagesMappings::compareTwoPaths(const QString &path1, const QString &path2) const {
    PathParsed p1(path1);
    PathParsed p2(path2);

    if (p1.type == p2.type || p1.type == PathParsed::Type::NONE || p2.type == PathParsed::Type::NONE) {
        const Name lowerPath1(p1.path);
        const Name lowerPath2(p2.path);
        const auto found1 = mappingsPages.find(lowerPath1);
        const auto found2 = mappingsPages.find(lowerPath2);
        if (found1 == mappingsPages.end() || found2 == mappingsPages.end()) {
            return lowerPath1 == lowerPath2;
        } else {
            return found1->second->page == found2->second->page;
        }
    } else {
        return false;
    }
}

const PageInfo& PagesMappings::getSearchPage() const {
    CHECK(!searchPage.page.isNull() && !searchPage.page.isEmpty(), "search page not found");
    return searchPage;
}

Optional<PageInfo> PagesMappings::findInternal(const QString &url) const {
    const auto found = mappingsPages.find(Name(url));
    if (found == mappingsPages.end()) {
        int foundSlash = -1;
        auto findSymbols = [](const QString &url, const QString &prefix) {
            int foundSlash = url.indexOf('/', prefix.size() + 1);
            int found2 = url.indexOf('#', prefix.size() + 1);
            if (found2 == -1) {
                found2 = url.size();
            }
            foundSlash = std::min(foundSlash, found2);
            return foundSlash;
        };

        if (url.startsWith(METAHASH_URL)) {
            foundSlash = findSymbols(url, METAHASH_URL);
        } else if (url.startsWith(APP_URL)) {
            foundSlash = findSymbols(url, APP_URL);
        } else {
            foundSlash = findSymbols(url, "");
        }
        if (foundSlash != -1) {
            const QString url2 = url.left(foundSlash);
            const auto found2 = mappingsPages.find(Name(url2));
            if (found2 == mappingsPages.end()) {
                return Optional<PageInfo>();
            } else {
                PageInfo page = *found2->second;
                page.page = concatenateTwoPath(page.page, url.mid(foundSlash));
                return page;
            }
        } else {
            return Optional<PageInfo>();
        }
    } else {
        return *found->second;
    }
}

const std::vector<QString>& PagesMappings::getDefaultIps() const {
    return defaultMhIps;
}

Optional<QString> PagesMappings::findName(const QString &url) const {
    const static QString HTTP_1 = "http://";
    const static QString HTTP_2 = "https://";

    auto found = urlToName.find(url);
    if (found != urlToName.end()) {
        return found->second;
    }

    auto findUrl = [this](const QString &findTxt, const QString &parameters) -> Optional<QString> {
        auto found = urlToName.find(findTxt);
        if (found != urlToName.end()) {
            return concatenateTwoPath(found->second, parameters);
        } else {
            return Optional<QString>();
        }
    };

    if (url.startsWith("file:")) {
        int findSharp = url.indexOf('#');
        if (findSharp == -1) {
            findSharp = url.size();
        }
        const QString url3 = url.left(findSharp);
        const int find = url3.lastIndexOf('/');
        if (find != -1) {
            Optional<QString> found = findUrl(url3.mid(find + 1), url.mid(findSharp));
            if (found.has_value()) {
                return found.value();
            }
        }
    } else {
        int foundSlash = -1;
        if (url.startsWith(HTTP_1)) {
            foundSlash = url.indexOf('/', HTTP_1.size() + 1);
        } else if (url.startsWith(HTTP_2)) {
            foundSlash = url.indexOf('/', HTTP_2.size() + 1);
        }
        if (foundSlash != -1) {
            Optional<QString> found = findUrl(url.left(foundSlash), url.mid(foundSlash));
            if (found.has_value()) {
                return found.value();
            }
        }
    }

    return Optional<QString>();
}

PageInfo PagesMappings::find(const QString &text) const {
    auto isFullUrl = [](const QString &text) {
        if (text.size() != 52) {
            return false;
        }
        if (!isHex(text.toStdString())) {
            return false;
        }
        return true;
    };

    PageInfo pageInfo;
    const auto found = findInternal(text);
    if (found.has_value()) {
        pageInfo = found.value();
    } else if (!text.startsWith(METAHASH_URL) && !text.startsWith(APP_URL)) {
        const QString appUrl = APP_URL + text;
        const auto found2 = findInternal(appUrl);
        if (found2.has_value()) {
            pageInfo = found2.value();
        } else if (isFullUrl(text)) {
            pageInfo.page = METAHASH_URL + text;
        }
    } else if (text.startsWith(METAHASH_URL)){
        pageInfo.page = text;
    } else {
        CHECK(text.startsWith(APP_URL), "Incorrect text: " + text.toStdString());
    }
    return pageInfo;
}

PagesMappings::UrlName::UrlName(const QString &name)
    : name(name.toLower())
{}

bool PagesMappings::UrlName::operator<(const PagesMappings::UrlName &second) const {
    return this->name < second.name;
}
